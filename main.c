#include <stdio.h>
#include<string.h>
#include<stdlib.h>

// blocos com tamanho de 10000 registros (para arq. indices)
#define BLOCK_SIZE 10000
#define PROD_FILE_NAME "sorted_products.bin"
#define EVENT_FILE_NAME "sorted_events.bin"
#define ORIGINAL_FILE_NAME "reducedData.csv"

// ordenado por id
typedef struct Product{
	int id;
	float price;
	char brand[15];
	char category_id[25];
	char category_code[50];	
	
} Product;

// ordenado por id
typedef struct Event{
	int id; // criado manualmente (arquivo original esta ordenado por data mas tem repeticao)
	int user_id;
	int product_id;
	char event_type[10];
	
} Event;

char *strsep(char **stringp, const char *delim) {
    char *rv = *stringp;
    if (rv) {
        *stringp += strcspn(*stringp, delim);
        if (**stringp)
            *(*stringp)++ = '\0';
        else
            *stringp = 0; }
    return rv;
}

//retorna o numero de linhas
int countNumLines(char *name) {
    FILE *file = fopen(name, "r");
    char line[250];
    int i = 0;

    if (file == NULL) {
        printf("Nao foi poss�vel abrir o arquivo");
        return -1;
    }

    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file) != NULL) {
        i++;
    }

    fclose(file);
    return i;
}

//----------------------------------
//INICIO PRODUTOS
//----------------------------------

// Fun��o de compara��o para ordenar os produtos pelo id
int compareProducts(const void *a, const void *b) {
    Product *prodA = (Product *)a;
    Product *prodB = (Product *)b;
    return prodA->id - prodB->id;
}

//retorna o numero de blocos
int createTemporaryProductFiles(char *filename) {
    FILE *file = fopen(filename, "r");
    char line[250];
    if (file == NULL) {
        perror("Nao foi poss�vel abrir o arquivo");
        return -1;
    }

    // Cabe�alho
    fgets(line, sizeof(line), file);
    
    int i = 0;
    int blockNumber = 0;
    
    Product *products = (Product *)malloc(sizeof(Product) * BLOCK_SIZE);
    if (products == NULL) {
        perror("Erro ao realocar mem�ria para produtos");
        fclose(file);
        return -1;
    }
    
    int b = 0;

    while (fgets(line, sizeof(line), file) != NULL) {		
        line[strcspn(line, "\n")] = '\0';

		char *token;
		char *rest = line;
		int j = 0;
		
		while ((token = strsep(&rest, ",")) != NULL) {
		    switch (j) {
		        case 2: // ID do Produto
		            products[b].id = atoi(token);
		            break;
		        case 3: // Categoria ID
		            strcpy(products[b].category_id, token);
		            break;
		        case 4: // C�digo da Categoria
		            if (token[0] == '\0') {
		                products[b].category_code[0] = '\0';
		            } else {
		                strcpy(products[b].category_code, token);
		            }
		            break;
		        case 5: // Marca
		            if (token[0] == '\0') {
		                products[b].brand[0] = '\0';
		            } else {
		                strcpy(products[b].brand, token);
		            }
		            break;
		        case 6: // Pre�o
		            if (token != NULL && strlen(token) > 0) {
		                products[b].price = strtod(token, NULL); // Converte string para float
		            } else {
		                products[b].price = 0.0;
		            }
		            break;
		    }
		    j++;
        }
        
        i++;
        b++;
        
        // Escreve os blocos.
        if (b == BLOCK_SIZE) {
            // Ordena o bloco
            qsort(products, b, sizeof(Product), compareProducts);
            
            // Cria um arquivo tempor�rio para o bloco
            char tempFileName[20];
            sprintf(tempFileName, "temp_p_block_%d.bin", blockNumber);
            FILE *tempFile = fopen(tempFileName, "wb");
            if (tempFile == NULL) {
                perror("Erro ao criar arquivo tempor�rio");
                free(products);
                fclose(file);
                return -1;
            }

            // Escreve o bloco ordenado no arquivo tempor�rio
            size_t written = fwrite(products, sizeof(Product), b, tempFile);
            fclose(tempFile);
            
            if (written != b) {
                printf("Erro ao escrever no arquivo tempor�rio. Esperado: %d, Escrito: %zu\n", b, written);
            } else {
                printf("Bloco %d escrito com sucesso em %s.\n", blockNumber, tempFileName);
            }

            blockNumber++;
            b = 0; // Reinicia o contador de produtos
        }
    }

    // Escreve remanescentes que n�o completam 1 bloco.
    if (b > 0) {
        qsort(products, b, sizeof(Product), compareProducts);
        
        char tempFileName[20];
        sprintf(tempFileName, "temp_p_block_%d.bin", blockNumber);
        FILE *tempFile = fopen(tempFileName, "wb");
        if (tempFile == NULL) {
            perror("Erro ao criar arquivo tempor�rio");
            free(products);
            fclose(file);
            return -1;
        }

        size_t written = fwrite(products, sizeof(Product), b, tempFile);
        fclose(tempFile);
        
        if (written != b) {
            printf("Erro ao escrever no arquivo tempor�rio. Esperado: %d, Escrito: %zu\n", b, written);
        } else {
            printf("Bloco %d escrito com sucesso em %s.\n", blockNumber, tempFileName);
        }
    }
    
    free(products);
    fclose(file);
    
    return blockNumber; // Retorna o n�mero de blocos criados
}

//realiza o merge dos arquivos temporarios
int mergeSortedBlocks(int numBlocks) {
    FILE *outputFile = fopen(PROD_FILE_NAME, "wb");
    if (outputFile == NULL) {
        perror("Nao foi possivel criar o arquivo de saida ordenado");
        return -1;
    }

    FILE *tempFiles[numBlocks];
    Product currentProducts[numBlocks];
    int activeFiles = 0;
	int i = 0;
    // Abre todos os arquivos tempor�rios
    for (i = 0; i < numBlocks; i++) {
        char tempFileName[20];
        sprintf(tempFileName, "temp_p_block_%d.bin", i);
        tempFiles[i] = fopen(tempFileName, "rb");
        if (tempFiles[i] != NULL) {
            if (fread(&currentProducts[i], sizeof(Product), 1, tempFiles[i]) == 1) {
                activeFiles++;
            }
        }
    }

    Product lastWritten; // guarda o ultimo produto escrito
    int firstWrite = 1;

    // Mesclagem dos blocos
    while (activeFiles > 0) {
        // Encontra o menor id entre os produtos atuais
        int minIndex = -1;
        int i;
        for (i = 0; i < numBlocks; i++) {
            if (tempFiles[i] != NULL && (minIndex == -1 || currentProducts[i].id < currentProducts[minIndex].id)) {
                minIndex = i;
            }
        }

        // Escreve o produto, sem duplicatas
        if (minIndex != -1) {
            if (firstWrite || currentProducts[minIndex].id != lastWritten.id) {
                fwrite(&currentProducts[minIndex], sizeof(Product), 1, outputFile);
                lastWritten = currentProducts[minIndex];
                firstWrite = 0;
            }

            // Carrega o pr�ximo produto do arquivo tempor�rio selecionado
            if (fread(&currentProducts[minIndex], sizeof(Product), 1, tempFiles[minIndex]) != 1) {
                fclose(tempFiles[minIndex]);
                tempFiles[minIndex] = NULL; // Fecha o arquivo e marcar como inativo
                activeFiles--;
            }
        }
    }

    fclose(outputFile);

    return 0;
}

void printProductsFromFile(char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        return;
    }
    
    Product product;
    size_t readCount;

    // L� os produtos do arquivo e imprime suas informa��es
    while ((readCount = fread(&product, sizeof(Product), 1, file)) == 1) {
        printf("ID: %d\n", product.id);
        printf("Price: %.2f\n", product.price);
        printf("Brand: %s\n", product.brand);
        printf("Category ID: %s\n", product.category_id);
        printf("Category Code: %s\n", product.category_code);
        printf("------------------------------\n");
    }

    if (readCount != 0) {
        perror("Erro ao ler o arquivo");
    }

    fclose(file);
}

//----------------------------------
//FINAL PRODUTOS
//----------------------------------

//retorna o sucesso ou falha
int createEventFile(char *filename) {
    FILE *file = fopen(filename, "r");
    FILE *fileEvents = fopen(EVENT_FILE_NAME, "wb");
    char line[250];
    if (file == NULL || fileEvents == NULL) {
        perror("Nao foi possivel abrir o arquivo");
        return 0;
    }

    // Cabe�alho
    fgets(line, sizeof(line), file);
    
    int i = 0;
    int blockNumber = 0;
    
    Event event;
    
    while (fgets(line, sizeof(line), file) != NULL) {		
        line[strcspn(line, "\n")] = '\0';

		char *token;
		char *rest = line;
		int j = 0;
		
		while ((token = strsep(&rest, ",")) != NULL) {
		    switch (j) {
		        case 1: // Tipo do Evento
		        	strcpy(event.event_type, token);
		            break;
		        case 2: // ID do Produto
		            event.product_id = atoi(token);
		            break;
		        case 7: // ID do usuario
		            event.user_id = atoi(token);
		            break;		        
		    }
		    j++;
        }
        
        event.id = i + 1; 	
		       
        fwrite(&event, sizeof(Event), 1, fileEvents);
        
        i++;
    }
      
    fclose(file);
    fclose(fileEvents);
    
    return 1; // Retorna sucesso
}


void printEventsFromFile(char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        return;
    }
    
    Event event;
    size_t readCount;

    // L� os produtos do arquivo e imprime suas informa��es
    while ((readCount = fread(&event, sizeof(Event), 1, file)) == 1) {
    	
        printf("ID: %d\n", event.id);
        printf("Event_type: %s\n", event.event_type);
        printf("Product ID: %d\n", event.product_id);
        printf("User ID: %d\n", event.user_id);
        printf("------------------------------\n");
    }

    if (readCount != 0) {
        perror("Erro ao ler o arquivo");
    }

    fclose(file);
}




void printMenu(){
	printf("---------------- MENU ----------------\n\n");
	printf("1 -> Gerar arquivos de dados\n");
	printf("2 -> Gerar arquivos de indice (chave)\n");
	printf("0 -> Sair\n");
	printf("Opcao: ");
}

int main(){
	int opc = 0;
	int numBlocks = 0;
	
	do{
		printMenu();
		scanf("%d", &opc);
		
		switch(opc){
			case 1:
				numBlocks = createTemporaryProductFiles(ORIGINAL_FILE_NAME);
			    if (numBlocks > 0) {
			        mergeSortedBlocks(numBlocks);
			        printf("Ordenacao completa e arquivo de produtos gerado.\n");
			    } else {
			        printf("Nenhum bloco de produtos foi criado.\n");
			    }
			    //printProductsFromFile(PROD_FILE_NAME);
			    createEventFile(ORIGINAL_FILE_NAME);
			    //printEventsFromFile(EVENT_FILE_NAME);
				break;
			case 2:
				
				break;
		}
		
		
	}while(opc != 0);
	
	
    return 0;
}

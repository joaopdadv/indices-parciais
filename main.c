#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// blocos com tamanho de 10000 registros (para arq. indices)
#define BLOCK_SIZE 100
#define PROD_FILE_NAME "sorted_products.bin"
#define PROD_IDINDEX_FILE_NAME "index_products.bin"
#define PROD_PRICE_INDEX_FILE_NAME "price_index_products.bin"
#define EVENT_FILE_NAME "sorted_events.bin"
#define EVENT_IDINDEX_FILE_NAME "index_events.bin"
#define EVENT_USER_INDEX_FILE_NAME "user_index_events.bin"
#define ORIGINAL_FILE_NAME "reducedData.csv"

// ordenado por id
typedef struct Product
{
    int id;
    float price;
    char brand[15];
    char category_id[25];
    char category_code[50];

} Product;

// ordenado por id
typedef struct Event
{
    int id; // criado manualmente (arquivo original esta ordenado por data mas tem repeticao)
    int user_id;
    int product_id;
    char event_type[10];

} Event;

typedef struct
{
    int block;
    int itemKey;
    int position;
} BlockIndex;


typedef struct
{
    float price;
    int position;
} PriceIndex;

typedef struct
{
    int user_id;
    int position;
} UserIDIndex;


char *strsep(char **stringp, const char *delim)
{
    char *rv = *stringp;
    if (rv)
    {
        *stringp += strcspn(*stringp, delim);
        if (**stringp)
            *(*stringp)++ = '\0';
        else
            *stringp = 0;
    }
    return rv;
}

// retorna o numero de linhas
int countNumLines(char *name)
{
    FILE *file = fopen(name, "r");
    char line[250];
    int i = 0;

    if (file == NULL)
    {
        printf("Nao foi possÃ­vel abrir o arquivo");
        return -1;
    }

    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file) != NULL)
    {
        i++;
    }

    fclose(file);
    return i;
}

//----------------------------------
// INICIO PRODUTOS
//----------------------------------

// Funcaoo de comparacao para ordenar os produtos pelo id
int compareProducts(const void *a, const void *b)
{
    Product *prodA = (Product *)a;
    Product *prodB = (Product *)b;
    return prodA->id - prodB->id;
}

// retorna o numero de blocos
int createTemporaryProductFiles(char *filename)
{
    FILE *file = fopen(filename, "r");
    char line[250];
    if (file == NULL)
    {
        perror("Nao foi possivel abrir o arquivo");
        return -1;
    }

    // Cabecaalho
    fgets(line, sizeof(line), file);

    int i = 0;
    int blockNumber = 0;

    Product *products = (Product *)malloc(sizeof(Product) * BLOCK_SIZE);
    if (products == NULL)
    {
        perror("Erro ao realocar memoria para produtos");
        fclose(file);
        return -1;
    }

    int b = 0;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        line[strcspn(line, "\n")] = '\0';

        char *token;
        char *rest = line;
        int j = 0;

        while ((token = strsep(&rest, ",")) != NULL)
        {
            switch (j)
            {
            case 2: // ID do Produto
                products[b].id = atoi(token);
                break;
            case 3: // Categoria ID
                strncpy(products[b].category_id, token, sizeof(products[b].category_id) - 1);
                products[b].category_id[sizeof(products[b].category_id) - 1] = '\0';

                // antes era:
                // strcpy(products[b].category_id, token);
                break;
            case 4: // Codigo da Categoria
                strncpy(products[b].category_code, token, sizeof(products[b].category_code) - 1);
                products[b].category_code[sizeof(products[b].category_code) - 1] = '\0';

                // antes era:
                // if (token[0] == '\0')
                // {
                //     products[b].category_code[0] = '\0';
                // }
                // else
                // {
                //     strcpy(products[b].category_code, token);
                // }

                break;
            case 5: // Marca
                strncpy(products[b].brand, token, sizeof(products[b].brand) - 1);
                products[b].brand[sizeof(products[b].brand) - 1] = '\0'; // Null-terminate

                // antes era:
                // if (token[0] == '\0')
                // {
                //     products[b].brand[0] = '\0';
                // }
                // else
                // {
                //     strcpy(products[b].brand, token);
                // }

                break;
            case 6: // Preco
                if (token != NULL && strlen(token) > 0)
                {
                    products[b].price = strtod(token, NULL); // Converte string para float
                }
                else
                {
                    products[b].price = 0.0;
                }
                break;
            }
            j++;
        }

        i++;
        b++;

        // Escreve os blocos.
        if (b == BLOCK_SIZE)
        {
            // Ordena o bloco
            qsort(products, b, sizeof(Product), compareProducts);

            // Cria um arquivo temporï¿½rio para o bloco
            char tempFileName[20];
            sprintf(tempFileName, "temp_p_block_%d.bin", blockNumber);
            FILE *tempFile = fopen(tempFileName, "wb");
            if (tempFile == NULL)
            {
                perror("Erro ao criar arquivo temporï¿½rio");
                free(products);
                fclose(file);
                return -1;
            }

            // Escreve o bloco ordenado no arquivo temporï¿½rio
            size_t written = fwrite(products, sizeof(Product), b, tempFile);
            fclose(tempFile);

            if (written != b)
            {
                printf("Erro ao escrever no arquivo temporÃ¡rio. Esperado: %d, Escrito: %zu\n", b, written);
            }
            else
            {
                printf("Bloco %d escrito com sucesso em %s.\n", blockNumber, tempFileName);
            }

            blockNumber++;
            b = 0; // Reinicia o contador de produtos
        }
    }

    // Escreve remanescentes que nï¿½o completam 1 bloco.
    if (b > 0)
    {
        qsort(products, b, sizeof(Product), compareProducts);

        char tempFileName[20];
        sprintf(tempFileName, "temp_p_block_%d.bin", blockNumber);
        FILE *tempFile = fopen(tempFileName, "wb");
        if (tempFile == NULL)
        {
            perror("Erro ao criar arquivo temporÃ¡rio");
            free(products);
            fclose(file);
            return -1;
        }

        size_t written = fwrite(products, sizeof(Product), b, tempFile);
        fclose(tempFile);

        if (written != b)
        {
            printf("Erro ao escrever no arquivo temporario. Esperado: %d, Escrito: %zu\n", b, written);
        }
        else
        {
            printf("Bloco %d escrito com sucesso em %s.\n", blockNumber, tempFileName);
        }
    }

    free(products);
    fclose(file);

    return blockNumber; // Retorna o nÃºmero de blocos criados
}

// realiza o merge dos arquivos temporarios
int mergeSortedBlocks(int numBlocks)
{
    FILE *outputFile = fopen(PROD_FILE_NAME, "wb");
    if (outputFile == NULL)
    {
        perror("Nao foi possivel criar o arquivo de saida ordenado");
        return -1;
    }

    FILE *tempFiles[numBlocks];
    Product currentProducts[numBlocks];
    int activeFiles = 0;
    int i = 0;
    // Abre todos os arquivos temporï¿½rios
    for (i = 0; i < numBlocks; i++)
    {
        char tempFileName[20];
        sprintf(tempFileName, "temp_p_block_%d.bin", i);
        tempFiles[i] = fopen(tempFileName, "rb");
        if (tempFiles[i] != NULL)
        {
            if (fread(&currentProducts[i], sizeof(Product), 1, tempFiles[i]) == 1)
            {
                activeFiles++;
            }
        }
    }

    Product lastWritten; // guarda o ultimo produto escrito
    int firstWrite = 1;

    // Mesclagem dos blocos
    while (activeFiles > 0)
    {
        // Encontra o menor id entre os produtos atuais
        int minIndex = -1;
        int i;
        for (i = 0; i < numBlocks; i++)
        {
            if (tempFiles[i] != NULL && (minIndex == -1 || currentProducts[i].id < currentProducts[minIndex].id))
            {
                minIndex = i;
            }
        }

        // Escreve o produto, sem duplicatas
        if (minIndex != -1)
        {
            if (firstWrite || currentProducts[minIndex].id != lastWritten.id)
            {
                fwrite(&currentProducts[minIndex], sizeof(Product), 1, outputFile);
                lastWritten = currentProducts[minIndex];
                firstWrite = 0;
            }

            // Carrega o prï¿½ximo produto do arquivo temporï¿½rio selecionado
            if (fread(&currentProducts[minIndex], sizeof(Product), 1, tempFiles[minIndex]) != 1)
            {
                fclose(tempFiles[minIndex]);
                tempFiles[minIndex] = NULL; // Fecha o arquivo e marcar como inativo
                activeFiles--;
            }
        }
    }

    fclose(outputFile);

    return 0;
}

void printProductsFromFile(char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("Erro ao abrir o arquivo");
        return;
    }

    Product product;
    size_t readCount;

    // Lï¿½ os produtos do arquivo e imprime suas informacoes
    while ((readCount = fread(&product, sizeof(Product), 1, file)) == 1)
    {
        printf("ID: %d\n", product.id);
        printf("Price: %.2f\n", product.price);
        printf("Brand: %s\n", product.brand);
        printf("Category ID: %s\n", product.category_id);
        printf("Category Code: %s\n", product.category_code);
        printf("------------------------------\n");
    }

    if (readCount != 0)
    {
        perror("Erro ao ler o arquivo");
    }

    fclose(file);
}

void createProductsIndexFile(char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("Nao foi possivel abrir o arquivo de produtos");
        return;
    }

    FILE *indexFile = fopen("index_products.bin", "w+b");
    if (indexFile == NULL)
    {
        perror("Nao foi possivel criar o arquivo de indices de produtos");
        return;
    }

    Product *product = (Product *)malloc(sizeof(Product));
    BlockIndex *i = (BlockIndex *)malloc(sizeof(BlockIndex));

    int block = 0;
    int totalProducts = 0;
    int indexCount = 0; // Para controlar a cada quantos IDs o indice sera criado

    while (fread(product, sizeof(Product), 1, file) == 1)
    {
        if (indexCount == BLOCK_SIZE)
        {
            i->position = totalProducts;
            i->block = block++;
            i->itemKey = product->id;

            fwrite(i, sizeof(BlockIndex), 1, indexFile);
            indexCount = 0;
        }

        indexCount++;
        totalProducts++;
    }

    // Cria o indice para o ultimo bloco de produto

    if (indexCount > 0)
    {
        i->block = block++;
        i->itemKey = product->id;
        i->position = totalProducts;
        fwrite(i, sizeof(BlockIndex), 1, indexFile);
    }

    printf("Total de blocos de indices criados (produtos): %d\n", block);

    fclose(file);
    fclose(indexFile);
    free(product);
    free(i);
}

int comparePrice(const void *a, const void *b) {
    PriceIndex *indexA = (PriceIndex *)a;
    PriceIndex *indexB = (PriceIndex *)b;

    if (indexA->price < indexB->price) return -1;
    if (indexA->price > indexB->price) return 1;
    return 0;
}

// Funcao para criar os arquivos temporarios de indice baseados no preco do produto (indice exaustivo) 
// (deve utilizar o arquivo de produtos)
int createTemporaryPriceIndexFiles(char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Nao foi possivel abrir o arquivo de produtos");
        return -1;
    }

    Product product;
    PriceIndex *indexArray = (PriceIndex *)malloc(sizeof(PriceIndex) * BLOCK_SIZE);
    if (indexArray == NULL) {
        perror("Erro ao alocar memoria para o bloco de indices");
        fclose(file);
        return -1;
    }

    int totalBlocks = 0;
    int indexCount = 0;
    int position = 0;
    int blockNumber = 0;

    // Ler o arquivo de produtos em blocos
    while (fread(&product, sizeof(Product), 1, file) == 1) {
        // Preencher o array de índices com o campo price e a posição
        indexArray[indexCount].price = product.price;
        indexArray[indexCount].position = position++;
        indexCount++;

        // Quando le a quantidade definida no tamanho do bloco, gera um arquivo temporario. 
        if (indexCount == BLOCK_SIZE) {
            qsort(indexArray, indexCount, sizeof(PriceIndex), comparePrice);

            // Criar um arquivo temporario para o bloco `blockNumber`
            char tempFileName[30];
            sprintf(tempFileName, "temp_price_block_%d.bin", blockNumber);
            FILE *tempFile = fopen(tempFileName, "wb");
            if (tempFile == NULL) {
                perror("Nao foi possivel criar o arquivo temporario");
                free(indexArray);
                fclose(file);
                return -1;
            }

            // Depois de ordenar, escreve os itens no arquivo temporario
            fwrite(indexArray, sizeof(PriceIndex), indexCount, tempFile);
            fclose(tempFile);
            blockNumber++;
            indexCount = 0; // Reinicia o contador de itens do proximo bloco
            
            // desaloca e realoca o array de index.
            free(indexArray);            
            indexArray = (PriceIndex *)malloc(sizeof(PriceIndex) * BLOCK_SIZE);
		    if (indexArray == NULL) {
		        perror("Erro ao alocar memoria para o bloco de indices");
		        fclose(file);
		        return -1;
		    }
        }
    }

    // Escreve os remanscentes que nao completam 1 bloco
    if (indexCount > 0) {
        qsort(indexArray, indexCount, sizeof(PriceIndex), comparePrice);

        char tempFileName[30];
        sprintf(tempFileName, "temp_price_block_%d.bin", blockNumber);
        FILE *tempFile = fopen(tempFileName, "wb");
        if (tempFile == NULL) {
            perror("Nao foi possivel criar o arquivo temporario");
            free(indexArray);
            fclose(file);
            return -1;
        }

        fwrite(indexArray, sizeof(PriceIndex), indexCount, tempFile);
        fclose(tempFile);
        blockNumber++;
    }

    free(indexArray);
    fclose(file);

    return blockNumber;
}

// Função para realizar o merge dos arquivos temporarios e gerar o arquivo final de indices por preco ordenado
// (abre todos os arquivos temporarios e como estes esta ordenados, le a primeira posicao de cada e vai escrevendo no arquivo final o menor indice encontrado.)
void mergeSortedPriceBlocks(int totalBlocks) {
    FILE *outputFile = fopen(PROD_PRICE_INDEX_FILE_NAME, "wb");
    if (outputFile == NULL) {
        perror("Nao foi possivel criar o arquivo final de indices ordenados");
        return;
    }

	
    FILE *tempFiles[totalBlocks]; // armazena o ponteiro para todos os arquivos temporarios.
    PriceIndex currentIndexes[totalBlocks]; // armazena o indice atual de cada arquivo
    int activeFiles = 0; // armazena a quantidade de arquivos abertos
    int i;

    // Abre todos os arquivos temporarios e le o primeiro indice de cada um
    for (i = 0; i < totalBlocks; i++) {
        char tempFileName[30];
        sprintf(tempFileName, "temp_price_block_%d.bin", i);
        tempFiles[i] = fopen(tempFileName, "rb");
        if (tempFiles[i] != NULL) {
        	// le o primeiro indice de cada arquivo temporario
            if (fread(&currentIndexes[i], sizeof(PriceIndex), 1, tempFiles[i]) == 1) {
                activeFiles++;
            }
        }
    }

    // Merge dos arquivos temporarios
    while (activeFiles > 0) {
        // Encontra o indice com o menor preco entre os arquivos ativos
        int minIndex = -1; // posicao do menor indice (preco)
        int i;
        for (i = 0; i < totalBlocks; i++) {
            if (tempFiles[i] != NULL && (minIndex == -1 || currentIndexes[i].price < currentIndexes[minIndex].price)) {
                minIndex = i;
            }
        }

        // Escrever o menor indice no arquivo final
        if (minIndex != -1) {
            fwrite(&currentIndexes[minIndex], sizeof(PriceIndex), 1, outputFile);

            // Carrega o proximo indice do arquivo temporario selecionado (o que ja foi escrito)
            if (fread(&currentIndexes[minIndex], sizeof(PriceIndex), 1, tempFiles[minIndex]) != 1) {
                // Se nao chegou ao final do arquivo atual, fecha o arquivo
                fclose(tempFiles[minIndex]);
                tempFiles[minIndex] = NULL; // faz o free do ponteiro do arquivo que foi fechado
                activeFiles--;
            }
        }
    }

    fclose(outputFile);
}

void printPriceIndexFromFile(char *indexFilename)
{
    FILE *indexFile = fopen(indexFilename, "rb");
    if (indexFile == NULL)
    {
        perror("Nao foi possivel abrir o arquivo de indice");
        return;
    }

    PriceIndex indexEntry;

    printf("Lista de Indices Baseados em Preco:\n");
    printf("-----------------------------------\n");

    while (fread(&indexEntry, sizeof(PriceIndex), 1, indexFile) == 1)
    {
        printf("Preco: %.2f, Posicao: %d\n", indexEntry.price, indexEntry.position);
    }

    fclose(indexFile);
}

int binarySearchIndexProducts(int id)
{
    FILE *indexFile = fopen(PROD_IDINDEX_FILE_NAME, "rb");
    if (indexFile == NULL)
    {
        perror("Erro ao abrir o arquivo de indices");
        return 0;
    }

    FILE *productFile = fopen(PROD_FILE_NAME, "rb");
    if (productFile == NULL)
    {
        perror("Erro ao abrir o arquivo de produtos");
        return 0;
    }

    int left = 0;
    fseek(indexFile, 0, SEEK_END);
    int right = ftell(indexFile) / sizeof(BlockIndex) - 1;
    int found = 0;
    BlockIndex index;

    while (left <= right && !found)
    {
        int mid = (left + right) / 2;

        fseek(indexFile, mid * sizeof(BlockIndex), SEEK_SET);
        fread(&index, sizeof(BlockIndex), 1, indexFile);

        // print index itemKey e id
        // printf("Item ID: %d, id: %d\n", index.itemKey, id);

        if (index.itemKey == id)
        {
            printf("Bloco: %d\n", index.block);
            printf("Item ID: %d\n", index.itemKey);
            printf("Position: %d\n", index.position);
            found = 1;
        }
        else if (index.itemKey > id)
        {
            // ler o anterior, se key for menor que id, entao eh o bloco que eu quero
            BlockIndex prevIndex;
            fseek(indexFile, (mid - 1) * sizeof(BlockIndex), SEEK_SET);
            fread(&prevIndex, sizeof(BlockIndex), 1, indexFile);

            if (prevIndex.itemKey < id)
            {
                printf("Bloco: %d\n", index.block);
                printf("Item ID: %d\n", index.itemKey);
                printf("Position: %d\n", index.position);
                found = 1;
            }
            else
            {
                right = mid - 1;
            }
        }
        else if (index.itemKey < id)
        {
            left = mid + 1;
        }
    }

    // quando encontrar o registro, ler o bloco (ir para o arquivo na posicao index.position - BLOCK_SIZE) e fazer a busca sequencial no arquivo normal atÃ© index.position
    if (!found)
    {
        return 0;
    }

    printf("------------------------------\n");

    right = index.position;
    left = index.position - BLOCK_SIZE;
    Product product;

    // printf("left: %d, right: %d\n", left, right);

    while (left <= right)
    {
        int mid = (left + right) / 2;
        fseek(productFile, mid * sizeof(Product), SEEK_SET);

        fread(&product, sizeof(Product), 1, productFile);

        if (product.id == id)
        {
            printf("ID: %d\n", product.id);
            printf("Price: %.2f\n", product.price);
            printf("Brand: %s\n", product.brand);
            printf("Category ID: %s\n", product.category_id);
            printf("Category Code: %s\n", product.category_code);
            return 1;
        }
        else if (product.id > id)
        {
            right = mid - 1;
        }
        else if (product.id < id)
        {
            left = mid + 1;
        }
    }

    fclose(indexFile);
    fclose(productFile);

    return 0;
}


int binarySearchPriceProducts(float price)
{
    FILE *indexFile = fopen(PROD_PRICE_INDEX_FILE_NAME, "rb");
    if (indexFile == NULL)
    {
        perror("Erro ao abrir o arquivo de indices");
        return 0;
    }

    FILE *productFile = fopen(PROD_FILE_NAME, "rb");
    if (productFile == NULL)
    {
        perror("Erro ao abrir o arquivo de produtos");
        return 0;
    }

    int left = 0;
    fseek(indexFile, 0, SEEK_END);
    int right = ftell(indexFile) / sizeof(PriceIndex) - 1;
    int found = 0;
    PriceIndex index;

    while (left <= right && !found)
    {
        int mid = (left + right) / 2;

        fseek(indexFile, mid * sizeof(PriceIndex), SEEK_SET);
        fread(&index, sizeof(PriceIndex), 1, indexFile);

        // print index itemKey e id
        // printf("Item ID: %d, id: %d\n", index.itemKey, id);

        if (index.price == price)
        {
            printf("Price: %.2f\n", index.price);
            printf("Position: %d\n", index.position);
            found = 1;
        }
        else if (index.price > price)
        {           
            right = mid - 1;
        }
        else if (index.price < price)
        {
            left = mid + 1;
        }
    }

    // quando encontrar o registro, utilizar o seek para encontrar no arquivo original de produtos pela position.
    if (!found)
    {
	    fclose(indexFile);
	    fclose(productFile);
        return 0;
    }

    printf("------------------------------\n");
    
    Product product;
	fseek(productFile, index.position * sizeof(Product), SEEK_SET);

    fread(&product, sizeof(Product), 1, productFile);

    printf("ID: %d\n", product.id);
    printf("Price: %.2f\n", product.price);
    printf("Brand: %s\n", product.brand);
    printf("Category ID: %s\n", product.category_id);
    printf("Category Code: %s\n", product.category_code);
    
    fclose(indexFile);
    fclose(productFile);
    
    return 1;

}



//----------------------------------
// FINAL PRODUTOS
//----------------------------------

//----------------------------------
// INICIO EVENTOS
//----------------------------------

// retorna o sucesso ou falha
int createEventFile(char *filename)
{
    FILE *file = fopen(filename, "r");
    FILE *fileEvents = fopen(EVENT_FILE_NAME, "wb");
    char line[250];
    if (file == NULL || fileEvents == NULL)
    {
        perror("Nao foi possivel abrir o arquivo");
        return 0;
    }

    // Cabeï¿½alho
    fgets(line, sizeof(line), file);

    int i = 0;
    int blockNumber = 0;

    Event event;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        line[strcspn(line, "\n")] = '\0';

        char *token;
        char *rest = line;
        int j = 0;

        while ((token = strsep(&rest, ",")) != NULL)
        {
            switch (j)
            {
            case 1: // Tipo do Evento
                strncpy(event.event_type, token, sizeof(event.event_type) - 1);
                event.event_type[sizeof(event.event_type) - 1] = '\0'; // Null-terminate

                // antes era:
                // strcpy(event.event_type, token);
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

void printEventsFromFile(char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("Erro ao abrir o arquivo");
        return;
    }

    Event event;
    size_t readCount;

    // Lï¿½ os produtos do arquivo e imprime suas informaï¿½ï¿½es
    while ((readCount = fread(&event, sizeof(Event), 1, file)) == 1)
    {

        printf("ID: %d\n", event.id);
        printf("Event_type: %s\n", event.event_type);
        printf("Product ID: %d\n", event.product_id);
        printf("User ID: %d\n", event.user_id);
        printf("------------------------------\n");
    }

    if (readCount != 0)
    {
        perror("Erro ao ler o arquivo");
    }

    fclose(file);
}

void createEventsIndexFile(char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("Nao foi possivel abrir o arquivo de eventos");
        return;
    }

    FILE *indexFile = fopen("index_events.bin", "w+b");
    if (indexFile == NULL)
    {
        perror("Nao foi possivel criar o arquivo de indices de eventos");
        return;
    }

    Event *event = (Event *)malloc(sizeof(Event));
    BlockIndex *i = (BlockIndex *)malloc(sizeof(BlockIndex));

    int block = 0;
    int totalEvents = 0;
    int indexCount = 0; // Para controlar a cada quantos IDs o inndice sera criado

    while (fread(event, sizeof(Event), 1, file) == 1)
    {
        if (indexCount == BLOCK_SIZE)
        {
            i->block = block++;
            i->itemKey = event->id;
            i->position = totalEvents;

            fwrite(i, sizeof(BlockIndex), 1, indexFile);
            indexCount = 0;
        }

        indexCount++;
        totalEvents++;
    }

    // Cria o indice para o ultimo bloco de evento
    if (indexCount > 0)
    {
        i->block = block++;
        i->itemKey = event->id;
        i->position = totalEvents;
        fwrite(i, sizeof(BlockIndex), 1, indexFile);
    }

    printf("Total de blocos de indices criados (eventos): %d\n", block);

    fclose(file);
    fclose(indexFile);
    free(event);
    free(i);
}

void printIndexFile(char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("Erro ao abrir o arquivo de indices");
        return;
    }

    BlockIndex index;
    size_t readCount;

    // Le os indices do arquivo e imprime suas informacoes
    while ((readCount = fread(&index, sizeof(BlockIndex), 1, file)) == 1)
    {
        printf("Bloco: %d\n", index.block);
        printf("Item ID: %d\n", index.itemKey);
        printf("Position: %d\n", index.position);
        printf("------------------------------\n");
    }

    if (readCount != 0)
    {
        perror("Erro ao ler o arquivo");
    }

    fclose(file);
}

int compareUserID(const void *a, const void *b) {
    UserIDIndex *indexA = (UserIDIndex *)a;
    UserIDIndex *indexB = (UserIDIndex *)b;

	return indexA->user_id - indexB->user_id;
}


// Funcao para criar os arquivos temporarios de indice baseados no id do usuario (indice exaustivo) 
// (deve utilizar o arquivo de eventos)
int createTemporaryUserIDIndexFiles(char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Nao foi possivel abrir o arquivo de produtos");
        return -1;
    }

    Event event;
    UserIDIndex *indexArray = (UserIDIndex *)malloc(sizeof(UserIDIndex) * BLOCK_SIZE);
    if (indexArray == NULL) {
        perror("Erro ao alocar memoria para o bloco de indices");
        fclose(file);
        return -1;
    }

    int totalBlocks = 0;
    int indexCount = 0;
    int position = 0;
    int blockNumber = 0;

    // Ler o arquivo de produtos em blocos
    while (fread(&event, sizeof(Event), 1, file) == 1) {
        // Preencher o array de índices com o campo price e a posição
        indexArray[indexCount].user_id = event.user_id;
        indexArray[indexCount].position = position++;
        indexCount++;

        // Quando le a quantidade definida no tamanho do bloco, gera um arquivo temporario. 
        if (indexCount == BLOCK_SIZE) {
            qsort(indexArray, indexCount, sizeof(UserIDIndex), compareUserID);

            // Criar um arquivo temporario para o bloco `blockNumber`
            char tempFileName[30];
            sprintf(tempFileName, "temp_user_id_block_%d.bin", blockNumber);
            FILE *tempFile = fopen(tempFileName, "wb");
            if (tempFile == NULL) {
                perror("Nao foi possivel criar o arquivo temporario");
                free(indexArray);
                fclose(file);
                return -1;
            }

            // Depois de ordenar, escreve os itens no arquivo temporario
            fwrite(indexArray, sizeof(UserIDIndex), indexCount, tempFile);
            fclose(tempFile);
            blockNumber++;
            indexCount = 0; // Reinicia o contador de itens do proximo bloco
            
            // desaloca e realoca o array de index.
            free(indexArray);            
            indexArray = (UserIDIndex *)malloc(sizeof(UserIDIndex) * BLOCK_SIZE);
		    if (indexArray == NULL) {
		        perror("Erro ao alocar memoria para o bloco de indices");
		        fclose(file);
		        return -1;
		    }
        }
    }

    // Escreve os remanscentes que nao completam 1 bloco
    if (indexCount > 0) {
        qsort(indexArray, indexCount, sizeof(UserIDIndex), compareUserID);

        char tempFileName[30];
        sprintf(tempFileName, "temp_user_id_block_%d.bin", blockNumber);
        FILE *tempFile = fopen(tempFileName, "wb");
        if (tempFile == NULL) {
            perror("Nao foi possivel criar o arquivo temporario");
            free(indexArray);
            fclose(file);
            return -1;
        }

        fwrite(indexArray, sizeof(UserIDIndex), indexCount, tempFile);
        fclose(tempFile);
        blockNumber++;
    }

    free(indexArray);
    fclose(file);

    return blockNumber;
}

// Função para realizar o merge dos arquivos temporarios e gerar o arquivo final de indices por preco ordenado
// (abre todos os arquivos temporarios e como estes esta ordenados, le a primeira posicao de cada e vai escrevendo no arquivo final o menor indice encontrado.)
void mergeSortedUserIDBlocks(int totalBlocks) {
    FILE *outputFile = fopen(EVENT_USER_INDEX_FILE_NAME, "wb");
    if (outputFile == NULL) {
        perror("Nao foi possivel criar o arquivo final de indices ordenados");
        return;
    }

	
    FILE *tempFiles[totalBlocks]; // armazena o ponteiro para todos os arquivos temporarios.
    UserIDIndex currentIndexes[totalBlocks]; // armazena o indice atual de cada arquivo
    int activeFiles = 0; // armazena a quantidade de arquivos abertos
    int i;

    // Abre todos os arquivos temporarios e le o primeiro indice de cada um
    for (i = 0; i < totalBlocks; i++) {
        char tempFileName[30];
        sprintf(tempFileName, "temp_user_id_block_%d.bin", i);
        tempFiles[i] = fopen(tempFileName, "rb");
        if (tempFiles[i] != NULL) {
        	// le o primeiro indice de cada arquivo temporario
            if (fread(&currentIndexes[i], sizeof(UserIDIndex), 1, tempFiles[i]) == 1) {
                activeFiles++;
            }
        }
    }

    // Merge dos arquivos temporarios
    while (activeFiles > 0) {
        // Encontra o indice com o menor preco entre os arquivos ativos
        int minIndex = -1; // posicao do menor indice (preco)
        int i;
        for (i = 0; i < totalBlocks; i++) {
            if (tempFiles[i] != NULL && (minIndex == -1 || currentIndexes[i].user_id < currentIndexes[minIndex].user_id)) {
                minIndex = i;
            }
        }

        // Escrever o menor indice no arquivo final
        if (minIndex != -1) {
            fwrite(&currentIndexes[minIndex], sizeof(UserIDIndex), 1, outputFile);

            // Carrega o proximo indice do arquivo temporario selecionado (o que ja foi escrito)
            if (fread(&currentIndexes[minIndex], sizeof(UserIDIndex), 1, tempFiles[minIndex]) != 1) {
                // Se nao chegou ao final do arquivo atual, fecha o arquivo
                fclose(tempFiles[minIndex]);
                tempFiles[minIndex] = NULL; // faz o free do ponteiro do arquivo que foi fechado
                activeFiles--;
            }
        }
    }

    fclose(outputFile);
}

void printUserIDIndexFromFile(char *indexFilename)
{
    FILE *indexFile = fopen(indexFilename, "rb");
    if (indexFile == NULL)
    {
        perror("Nao foi possivel abrir o arquivo de indice");
        return;
    }

    UserIDIndex indexEntry;

    printf("Lista de Indices Baseados em user_ID:\n");
    printf("-----------------------------------\n");

    while (fread(&indexEntry, sizeof(UserIDIndex), 1, indexFile) == 1)
    {
        printf("user_id: %d, Posicao: %d\n", indexEntry.user_id, indexEntry.position);
    }

    fclose(indexFile);
}


int binarySearchIndexEvents(int id)
{
    FILE *indexFile = fopen(EVENT_IDINDEX_FILE_NAME, "rb");
    if (indexFile == NULL)
    {
        perror("Erro ao abrir o arquivo de indices");
        return 0;
    }

    FILE *eventsFile = fopen(EVENT_FILE_NAME, "rb");
    if (eventsFile == NULL)
    {
        perror("Erro ao abrir o arquivo de produtos");
        return 0;
    }

    int left = 0;
    fseek(indexFile, 0, SEEK_END);
    int right = ftell(indexFile) / sizeof(BlockIndex) - 1;
    int found = 0;
    BlockIndex index;

    while (left <= right && !found)
    {
        int mid = (left + right) / 2;

        fseek(indexFile, mid * sizeof(BlockIndex), SEEK_SET);
        fread(&index, sizeof(BlockIndex), 1, indexFile);

        // print index itemKey e id
        // printf("Item ID: %d, id: %d\n", index.itemKey, id);

        if (index.itemKey == id)
        {
            printf("Bloco: %d\n", index.block);
            printf("Item ID: %d\n", index.itemKey);
            printf("Position: %d\n", index.position);
            found = 1;
        }
        else if (index.itemKey > id)
        {
            // ler o anterior, se key for menor que id, entao eh o bloco que eu quero
            BlockIndex prevIndex;
            fseek(indexFile, (mid - 1) * sizeof(BlockIndex), SEEK_SET);
            fread(&prevIndex, sizeof(BlockIndex), 1, indexFile);

            if (prevIndex.itemKey < id)
            {
                printf("Bloco: %d\n", index.block);
                printf("Item ID: %d\n", index.itemKey);
                printf("Position: %d\n", index.position);
                found = 1;
            }
            else
            {
                right = mid - 1;
            }
        }
        else if (index.itemKey < id)
        {
            left = mid + 1;
        }
    }

    // quando encontrar o registro, ler o bloco (ir para o arquivo na posicao index.position - BLOCK_SIZE) e fazer a busca sequencial no arquivo normal ate index.position
    if (!found)
    {
        return 0;
    }

    printf("------------------------------\n");

    right = index.position;
    left = index.position - BLOCK_SIZE;
    Event event;

    // printf("left: %d, right: %d\n", left, right);

    while (left <= right)
    {
        int mid = (left + right) / 2;
        fseek(eventsFile, mid * sizeof(Event), SEEK_SET);

        fread(&event, sizeof(Event), 1, eventsFile);

        if (event.id == id)
        {
            printf("ID: %d\n", event.id);
            printf("Type: %s\n", event.event_type);
            printf("Product ID: %d\n", event.product_id);
            printf("User ID: %d\n", event.user_id);
            return 1;
        }
        else if (event.id > id)
        {
            right = mid - 1;
        }
        else if (event.id < id)
        {
            left = mid + 1;
        }
    }

    fclose(indexFile);
    fclose(eventsFile);

    return 0;
}

//----------------------------------
// FINAL EVENTOS
//----------------------------------

void printMenu()
{
    printf("---------------- MENU ----------------\n");
    printf("1 -> Gerar arquivos de dados\n");
    printf("2 -> Gerar arquivos de indice (chave)\n");
    printf("3 -> Gerar arquivos de indice (produto -> por preco, evento -> por user_id)\n");
    printf("4 -> Pesquisa binaria por ID em produtos\n");
    printf("5 -> Pesquisa binaria por ID em eventos\n");
    printf("6 -> Pesquisa binaria por preco em produtos\n");
    printf("7 -> Pesquisa binaria por user_id em eventos\n");
    printf("0 -> Sair\n");
    printf("Opcao: ");
}

int main()
{
    int opc = 0;
    int numBlocks = 0;

    do
    {
        printMenu();
        scanf("%d", &opc);

        switch (opc)
        {
        case 1:
            numBlocks = createTemporaryProductFiles(ORIGINAL_FILE_NAME);
            if (numBlocks > 0)
            {
                mergeSortedBlocks(numBlocks);
                printf("Ordenacao completa e arquivo de produtos gerado.\n");
            }
            else
            {
                printf("Nenhum bloco de produtos foi criado.\n");
            }
            // printProductsFromFile(PROD_FILE_NAME);
            createEventFile(ORIGINAL_FILE_NAME);
            // printEventsFromFile(EVENT_FILE_NAME);
            break;
        case 2:

            createProductsIndexFile(PROD_FILE_NAME);
            printIndexFile(PROD_IDINDEX_FILE_NAME);
            createEventsIndexFile(EVENT_FILE_NAME);
            // printIndexFile(EVENT_IDINDEX_FILE_NAME);

            break;
        case 3:
        	numBlocks = createTemporaryPriceIndexFiles(PROD_FILE_NAME);
		    if (numBlocks > 0) {
		        mergeSortedPriceBlocks(numBlocks);
		        printf("Arquivo de indice ordenado por preco criado com sucesso.\n");
		        //printPriceIndexFromFile(PROD_PRICE_INDEX_FILE_NAME);
		    } else {
		        printf("Erro ao criar os arquivos temporarios de indice.\n");
		    }
		    
        	numBlocks = createTemporaryUserIDIndexFiles(EVENT_FILE_NAME);
		    if (numBlocks > 0) {
		        mergeSortedUserIDBlocks(numBlocks);
		        printf("Arquivo de indice ordenado por user_id criado com sucesso.\n");
		        //printUserIDIndexFromFile(EVENT_USER_INDEX_FILE_NAME);
		    } else {
		        printf("Erro ao criar os arquivos temporarios de indice.\n");
		    }		    
		    
		    
        	break;
        case 4:
            // int id;
            // printf("Digite o ID do produto: ");
            // scanf("%d", &id);
            // binarySearchIndexProducts(id);
            binarySearchIndexProducts(50600085); // 50600085 como exemplo
            break;
        case 5:
            // int id;
            // printf("Digite o ID do evento: ");
            // scanf("%d", &id);
            // binarySearchIndexEvents(id);
            binarySearchIndexEvents(1205); // 1205 como exemplo
            break;
        case 6:
            // float price;
            // printf("Digite o preco do produto: ");
            // scanf("%f", &price);
            // binarySearchPriceProducts(price);
        	binarySearchPriceProducts(5.77); // 5.77 como exemplo
        	break;
        }

    } while (opc != 0);

    return 0;
}

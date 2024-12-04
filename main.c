#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// blocos com tamanho de 10000 registros (para arq. indices)
#define BLOCK_SIZE 1000000
#define PROD_FILE_NAME "sorted_products.bin"
#define PROD_IDINDEX_FILE_NAME "index_products.bin"
#define PROD_PRICE_INDEX_FILE_NAME "price_index_products.bin"
#define EVENT_FILE_NAME "sorted_events.bin"
#define EVENT_IDINDEX_FILE_NAME "index_events.bin"
#define EVENT_USER_INDEX_FILE_NAME "user_index_events.bin"
#define ORIGINAL_FILE_NAME "2019-Nov.csv"
//#define ORIGINAL_FILE_NAME "reducedData.csv"

#define ORDER 1024  // Ordem da Arvore B

#define HASH_SIZE 1000000
#define HASH_INDEX_FILE_NAME "hash_events_user_index.bin"

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
    int position;
    float price;
} PriceIndex;

typedef struct
{
    int user_id;
    int position;
} UserIDIndex;

typedef struct
{
    int hash;
    int position;
} MemoryIndex;

MemoryIndex *hashs;

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
                perror("Erro ao criar arquivo temporario");
                free(products);
                fclose(file);
                return -1;
            }

            // Escreve o bloco ordenado no arquivo temporario
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

            blockNumber++;
            b = 0; // Reinicia o contador de produtos
        }
    }

    // Escreve remanescentes que nao completam 1 bloco.
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

int comparePrice(const void *a, const void *b)
{
    PriceIndex *indexA = (PriceIndex *)a;
    PriceIndex *indexB = (PriceIndex *)b;

    if (indexA->price < indexB->price)
        return -1;
    if (indexA->price > indexB->price)
        return 1;
    return 0;
}

// Funcao para criar os arquivos temporarios de indice baseados no preco do produto (indice exaustivo)
// (deve utilizar o arquivo de produtos)
int createTemporaryPriceIndexFiles(char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("Nao foi possivel abrir o arquivo de produtos");
        return -1;
    }

    Product product;
    PriceIndex *indexArray = (PriceIndex *)malloc(sizeof(PriceIndex) * BLOCK_SIZE);
    if (indexArray == NULL)
    {
        perror("Erro ao alocar memoria para o bloco de indices");
        fclose(file);
        return -1;
    }

    int totalBlocks = 0;
    int indexCount = 0;
    int position = 0;
    int blockNumber = 0;

    // Ler o arquivo de produtos em blocos
    while (fread(&product, sizeof(Product), 1, file) == 1)
    {
        // Preencher o array de ï¿½ndices com o campo price e a posicao
        indexArray[indexCount].price = product.price;
        indexArray[indexCount].position = position++;
        indexCount++;

        // Quando le a quantidade definida no tamanho do bloco, gera um arquivo temporario.
        if (indexCount == BLOCK_SIZE)
        {
            qsort(indexArray, indexCount, sizeof(PriceIndex), comparePrice);

            // Criar um arquivo temporario para o bloco `blockNumber`
            char tempFileName[30];
            sprintf(tempFileName, "temp_price_block_%d.bin", blockNumber);
            FILE *tempFile = fopen(tempFileName, "wb");
            if (tempFile == NULL)
            {
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
            if (indexArray == NULL)
            {
                perror("Erro ao alocar memoria para o bloco de indices");
                fclose(file);
                return -1;
            }
        }
    }

    // Escreve os remanscentes que nao completam 1 bloco
    if (indexCount > 0)
    {
        qsort(indexArray, indexCount, sizeof(PriceIndex), comparePrice);

        char tempFileName[30];
        sprintf(tempFileName, "temp_price_block_%d.bin", blockNumber);
        FILE *tempFile = fopen(tempFileName, "wb");
        if (tempFile == NULL)
        {
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

// Funï¿½ï¿½o para realizar o merge dos arquivos temporarios e gerar o arquivo final de indices por preco ordenado
// (abre todos os arquivos temporarios e como estes esta ordenados, le a primeira posicao de cada e vai escrevendo no arquivo final o menor indice encontrado.)
void mergeSortedPriceBlocks(int totalBlocks)
{
    FILE *outputFile = fopen(PROD_PRICE_INDEX_FILE_NAME, "wb");
    if (outputFile == NULL)
    {
        perror("Nao foi possivel criar o arquivo final de indices ordenados");
        return;
    }

    FILE *tempFiles[totalBlocks];           // armazena o ponteiro para todos os arquivos temporarios.
    PriceIndex currentIndexes[totalBlocks]; // armazena o indice atual de cada arquivo
    int activeFiles = 0;                    // armazena a quantidade de arquivos abertos
    int i;

    // Abre todos os arquivos temporarios e le o primeiro indice de cada um
    for (i = 0; i < totalBlocks; i++)
    {
        char tempFileName[30];
        sprintf(tempFileName, "temp_price_block_%d.bin", i);
        tempFiles[i] = fopen(tempFileName, "rb");
        if (tempFiles[i] != NULL)
        {
            // le o primeiro indice de cada arquivo temporario
            if (fread(&currentIndexes[i], sizeof(PriceIndex), 1, tempFiles[i]) == 1)
            {
                activeFiles++;
            }
        }
    }

    // Merge dos arquivos temporarios
    while (activeFiles > 0)
    {
        // Encontra o indice com o menor preco entre os arquivos ativos
        int minIndex = -1; // posicao do menor indice (preco)
        int i;
        for (i = 0; i < totalBlocks; i++)
        {
            if (tempFiles[i] != NULL && (minIndex == -1 || currentIndexes[i].price < currentIndexes[minIndex].price))
            {
                minIndex = i;
            }
        }

        // Escrever o menor indice no arquivo final
        if (minIndex != -1)
        {
            fwrite(&currentIndexes[minIndex], sizeof(PriceIndex), 1, outputFile);

            // Carrega o proximo indice do arquivo temporario selecionado (o que ja foi escrito)
            if (fread(&currentIndexes[minIndex], sizeof(PriceIndex), 1, tempFiles[minIndex]) != 1)
            {
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

void sequencialSearchProductsId(int id)
{
    FILE *file = fopen(PROD_FILE_NAME, "rb");
    if (file == NULL)
    {
        perror("Erro ao abrir o arquivo de produtos");
        return;
    }

    Product product;
    int found = 0;

    while (fread(&product, sizeof(Product), 1, file) == 1)
    {
        if (product.id == id)
        {
            printf("ID: %d\n", product.id);
            printf("Price: %.2f\n", product.price);
            printf("Brand: %s\n", product.brand);
            printf("Category ID: %s\n", product.category_id);
            printf("Category Code: %s\n", product.category_code);
            found = 1;
            break;
        }
    }

    if (!found)
    {
        printf("Produto nao encontrado.\n");
    }

    fclose(file);
}


void sequencialSearchProductsPrice(float price)
{
    FILE *file = fopen(PROD_FILE_NAME, "rb");
    if (file == NULL)
    {
        perror("Erro ao abrir o arquivo de produtos");
        return;
    }

    Product product;
    int found = 0;

    while (fread(&product, sizeof(Product), 1, file) == 1)
    {
        if (product.price == price)
        {
            printf("ID: %d\n", product.id);
            printf("Price: %.2f\n", product.price);
            printf("Brand: %s\n", product.brand);
            printf("Category ID: %s\n", product.category_id);
            printf("Category Code: %s\n", product.category_code);
            found = 1;
            break;
        }
    }

    if (!found)
    {
        printf("Produto nao encontrado.\n");
    }

    fclose(file);
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

int compareUserID(const void *a, const void *b)
{
    UserIDIndex *indexA = (UserIDIndex *)a;
    UserIDIndex *indexB = (UserIDIndex *)b;

    return indexA->user_id - indexB->user_id;
}

// Funcao para criar os arquivos temporarios de indice baseados no id do usuario (indice exaustivo)
// (deve utilizar o arquivo de eventos)
int createTemporaryUserIDIndexFiles(char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("Nao foi possivel abrir o arquivo de produtos");
        return -1;
    }

    Event event;
    UserIDIndex *indexArray = (UserIDIndex *)malloc(sizeof(UserIDIndex) * BLOCK_SIZE);
    if (indexArray == NULL)
    {
        perror("Erro ao alocar memoria para o bloco de indices");
        fclose(file);
        return -1;
    }

    int totalBlocks = 0;
    int indexCount = 0;
    int position = 0;
    int blockNumber = 0;

    // Ler o arquivo de produtos em blocos
    while (fread(&event, sizeof(Event), 1, file) == 1)
    {
        // Preencher o array de ï¿½ndices com o campo price e a posiï¿½ï¿½o
        indexArray[indexCount].user_id = event.user_id;
        indexArray[indexCount].position = position++;
        indexCount++;

        // Quando le a quantidade definida no tamanho do bloco, gera um arquivo temporario.
        if (indexCount == BLOCK_SIZE)
        {
            qsort(indexArray, indexCount, sizeof(UserIDIndex), compareUserID);

            // Criar um arquivo temporario para o bloco `blockNumber`
            char tempFileName[30];
            sprintf(tempFileName, "temp_user_id_block_%d.bin", blockNumber);
            FILE *tempFile = fopen(tempFileName, "wb");
            if (tempFile == NULL)
            {
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
            if (indexArray == NULL)
            {
                perror("Erro ao alocar memoria para o bloco de indices");
                fclose(file);
                return -1;
            }
        }
    }

    // Escreve os remanscentes que nao completam 1 bloco
    if (indexCount > 0)
    {
        qsort(indexArray, indexCount, sizeof(UserIDIndex), compareUserID);

        char tempFileName[30];
        sprintf(tempFileName, "temp_user_id_block_%d.bin", blockNumber);
        FILE *tempFile = fopen(tempFileName, "wb");
        if (tempFile == NULL)
        {
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

// Funï¿½ï¿½o para realizar o merge dos arquivos temporarios e gerar o arquivo final de indices por preco ordenado
// (abre todos os arquivos temporarios e como estes esta ordenados, le a primeira posicao de cada e vai escrevendo no arquivo final o menor indice encontrado.)
void mergeSortedUserIDBlocks(int totalBlocks)
{
    FILE *outputFile = fopen(EVENT_USER_INDEX_FILE_NAME, "wb");
    if (outputFile == NULL)
    {
        perror("Nao foi possivel criar o arquivo final de indices ordenados");
        return;
    }

    FILE *tempFiles[totalBlocks];            // armazena o ponteiro para todos os arquivos temporarios.
    UserIDIndex currentIndexes[totalBlocks]; // armazena o indice atual de cada arquivo
    int activeFiles = 0;                     // armazena a quantidade de arquivos abertos
    int i;

    // Abre todos os arquivos temporarios e le o primeiro indice de cada um
    for (i = 0; i < totalBlocks; i++)
    {
        char tempFileName[30];
        sprintf(tempFileName, "temp_user_id_block_%d.bin", i);
        tempFiles[i] = fopen(tempFileName, "rb");
        if (tempFiles[i] != NULL)
        {
            // le o primeiro indice de cada arquivo temporario
            if (fread(&currentIndexes[i], sizeof(UserIDIndex), 1, tempFiles[i]) == 1)
            {
                activeFiles++;
            }
        }
    }

    // Merge dos arquivos temporarios
    while (activeFiles > 0)
    {
        // Encontra o indice com o menor preco entre os arquivos ativos
        int minIndex = -1; // posicao do menor indice (preco)
        int i;
        for (i = 0; i < totalBlocks; i++)
        {
            if (tempFiles[i] != NULL && (minIndex == -1 || currentIndexes[i].user_id < currentIndexes[minIndex].user_id))
            {
                minIndex = i;
            }
        }

        // Escrever o menor indice no arquivo final
        if (minIndex != -1)
        {
            fwrite(&currentIndexes[minIndex], sizeof(UserIDIndex), 1, outputFile);

            // Carrega o proximo indice do arquivo temporario selecionado (o que ja foi escrito)
            if (fread(&currentIndexes[minIndex], sizeof(UserIDIndex), 1, tempFiles[minIndex]) != 1)
            {
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

int binarySearchUserIDEvents(int user_id)
{
    FILE *indexFile = fopen(EVENT_USER_INDEX_FILE_NAME, "rb");
    if (indexFile == NULL)
    {
        perror("Erro ao abrir o arquivo de indices");
        return 0;
    }

    FILE *eventsFile = fopen(EVENT_FILE_NAME, "rb");
    if (eventsFile == NULL)
    {
        perror("Erro ao abrir o arquivo de eventos");
        return 0;
    }

    int left = 0;
    fseek(indexFile, 0, SEEK_END);
    int right = ftell(indexFile) / sizeof(UserIDIndex) - 1;
    int found = 0;
    UserIDIndex index;

    while (left <= right && !found)
    {
        int mid = (left + right) / 2;

        fseek(indexFile, mid * sizeof(UserIDIndex), SEEK_SET);
        fread(&index, sizeof(UserIDIndex), 1, indexFile);

        // print index itemKey e id
        // printf("Item ID: %d, id: %d\n", index.itemKey, id);

        if (index.user_id == user_id)
        {
            printf("User ID: %d\n", index.user_id);
            printf("Position: %d\n", index.position);
            found = 1;
        }
        else if (index.user_id > user_id)
        {
            right = mid - 1;
        }
        else if (index.user_id < user_id)
        {
            left = mid + 1;
        }
    }

    // quando encontrar o registro, utilizar o seek para encontrar no arquivo original de produtos pela position.
    if (!found)
    {
        fclose(indexFile);
        fclose(eventsFile);
        return 0;
    }

    printf("------------------------------\n");

    Event event;
    fseek(eventsFile, index.position * sizeof(Event), SEEK_SET);

    fread(&event, sizeof(Event), 1, eventsFile);

    printf("ID: %d\n", event.id);
    printf("Type: %s\n", event.event_type);
    printf("Product ID: %d\n", event.product_id);
    printf("User ID: %d\n", event.user_id);

    fclose(indexFile);
    fclose(eventsFile);

    return 1;
}

//----------------------------------
// FINAL EVENTOS
//----------------------------------


// ------------- Trab 2 -------------


// Definindo a estrutura de um nó da árvore B
typedef struct BTreeNode {
    int num_keys;                      // Número de chaves no nó
    int* keys;                         // Chaves do nó
    int* values;                      // Ponteiros para os registros (endereços)
    struct BTreeNode** children;       // Ponteiros para os filhos
    int is_leaf;                       // 1 se for folha, 0 caso contrário
} BTreeNode;

// Definindo a árvore B
typedef struct BTree {
    BTreeNode* root;
} BTree;

// Função para criar um nó da árvore B
BTreeNode* create_node(int is_leaf) {
    BTreeNode* node = (BTreeNode*)malloc(sizeof(BTreeNode));
    node->is_leaf = is_leaf;
    node->num_keys = 0;
    node->keys = (int*)malloc(sizeof(int) * (ORDER - 1));
    node->values = (int*)malloc(sizeof(int) * (ORDER - 1));
    node->children = (BTreeNode**)malloc(sizeof(BTreeNode*) * ORDER);
    return node;
}

// Função para criar a árvore B
BTree* create_btree() {
    BTree* tree = (BTree*)malloc(sizeof(BTree));
    tree->root = create_node(1);  // A raiz inicialmente é uma folha
    return tree;
}

// Função para dividir um nó cheio
void split_child(BTreeNode* parent, int index) {
    BTreeNode* full_node = parent->children[index];
    BTreeNode* new_node = create_node(full_node->is_leaf);

    int mid = ORDER / 2;

    // Movendo a chave do meio para o nó pai
    parent->keys[index] = full_node->keys[mid];
    parent->num_keys++;

    // Transferindo a segunda metade das chaves e filhos para o novo nó
    int i = mid + 1;
    for (i; i < ORDER - 1; i++) {
        new_node->keys[i - (mid + 1)] = full_node->keys[i];
        new_node->values[i - (mid + 1)] = full_node->values[i];
        new_node->children[i - (mid + 1)] = full_node->children[i];
        full_node->keys[i] = 0;
        full_node->values[i] = 0;
        full_node->children[i] = NULL;
    }

    full_node->num_keys = mid;

    // Atualizando o filho da árvore
    parent->children[index + 1] = new_node;
}

// Função para inserir no nó não cheio
void insert_non_full(BTreeNode* node, int id, int value) {
    int i = node->num_keys - 1;

    // Se for uma folha, insere diretamente
    if (node->is_leaf) {
        while (i >= 0 && node->keys[i] > id) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }

        node->keys[i + 1] = id;
        node->values[i + 1] = value;
        node->num_keys++;
    } else {
        // Se não for uma folha, procura o filho correto
        while (i >= 0 && node->keys[i] > id) {
            i--;
        }
        i++;

        // Chama recursivamente para o filho
        if (node->children[i]->num_keys == ORDER - 1) {
            split_child(node, i);
            if (node->keys[i] < id) {
                i++;
            }
        }
        insert_non_full(node->children[i], id, value);
    }
}


// Função para realizar a inserção na árvore B
void insert(BTree* tree, int id, int value) {
    BTreeNode* root = tree->root;

    // Se a raiz estiver cheia, devemos dividir
    if (root->num_keys == ORDER - 1) {
        BTreeNode* new_root = create_node(0);  // Novo nó raiz não é folha
        new_root->children[0] = root;

        // Dividindo a raiz
        split_child(new_root, 0);
        tree->root = new_root;
    }

    // Inserir no nó correto
    insert_non_full(tree->root, id, value);
}

// Função para pesquisar recursivamente em um nó
int search_node(BTreeNode* node, int id) {
    int i = 0;

    // Procurando no nó atual a chave correspondente
    while (i < node->num_keys && id > node->keys[i]) {
        i++;
    }

    // Se encontramos a chave
    if (i < node->num_keys && id == node->keys[i]) {
        return node->values[i];  // Retorna o endereço do registro no arquivo
    }

    // Se o nó é uma folha, a chave não existe
    if (node->is_leaf) {
        return (int)-1;  // Não encontrado
    }

    // Se não encontramos a chave, seguimos na subárvore
    return search_node(node->children[i], id);
}


// Função para inserir um produto no índice
void insert_product_index(BTree* tree, Product* product, int file_offset) {
    insert(tree, product->id, file_offset);
}

// Função para realizar a pesquisa na árvore B
void search(BTree* tree, int id) {	
	
	int position = search_node(tree->root, id);
			
	 // quando encontrar o registro, utilizar o seek para encontrar no arquivo original de produtos pela position.
    if (position < 0)
    {
        return;
    }

    printf("------------------------------\n");
    
	FILE *file = fopen(PROD_FILE_NAME, "rb");
    if (file == NULL)
    {
        perror("Erro ao abrir o arquivo de produtos");
        return;
    }

    Product product;	
    fseek(file, position * sizeof(Product), SEEK_SET);

    fread(&product, sizeof(Product), 1, file);

    printf("ID: %d\n", product.id);
    printf("Price: %.2f\n", product.price);
    printf("Brand: %s\n", product.brand);
    printf("Category ID: %s\n", product.category_id);
    printf("Category Code: %s\n", product.category_code);

    fclose(file);
	
}

// ------------- Trab 2 -------------

void createBTreeIndexFile(char *filename, BTree* tree)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("Nao foi possivel abrir o arquivo de produtos");
        return;
    }

    Product *product = (Product *)malloc(sizeof(Product));
    int totalProducts = 0;
    
    while (fread(product, sizeof(Product), 1, file) == 1)
    {    	
		insert_product_index(tree, product, totalProducts);		   	
        totalProducts++;
    }

    fclose(file);
    free(product);
}

void initialize_memory_index(MemoryIndex **hashs)
{
    *hashs = (MemoryIndex *)malloc(HASH_SIZE * sizeof(MemoryIndex));
    if (*hashs == NULL)
    {
        perror("Erro ao alocar memoria");
        exit(EXIT_FAILURE);
    }
	int i;
    for (i = 0; i < HASH_SIZE; i++)
    {
        (*hashs)[i].hash = 0; // Marca como vazio
        (*hashs)[i].position = -1;
    }
}

void insert_memory_index(int user_id, int position, MemoryIndex *hashs)
{
    int hash1 = user_id % HASH_SIZE;
    int hash2 = 1 + (user_id % (HASH_SIZE - 1));
    int hash = hash1;

    printf("Hash: %d\n", hash);
    printf("User ID: %d\n", user_id);

    int i = 0;
    while (1)
    {
        if (hashs[hash].hash == 0) // Slot vazio
        {
            hashs[hash].hash = user_id;
            hashs[hash].position = position;
            break;
        }

        i++;
        hash = (hash1 + i * hash2) % HASH_SIZE;

        if (i >= HASH_SIZE)
        {
            printf("Erro: Indice esta cheio.\n");
            break;
        }
    }
}

void createHashIndexes(char *filename, MemoryIndex *hashs)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("Nao foi possivel abrir o arquivo");
        return;
    }

    Event *event = (Event *)malloc(sizeof(Event));
    int totalEvents = 0;
    
    while (fread(event, sizeof(Event), 1, file) == 1)
    {    	
        // Inserir no hash
        insert_memory_index(event->user_id, totalEvents, hashs);		   	
        totalEvents++;
    }

    fclose(file);
    free(event);
}

MemoryIndex get_memory_index(int user_id, MemoryIndex *hashs)
{
    int hash1 = user_id % HASH_SIZE;
    int hash2 = 1 + (user_id % (HASH_SIZE - 1));
    int hash = hash1;

    printf("Hash: %d\n", hash);
    printf("User ID: %d\n", user_id);

    int i = 0;
    while (1)
    {
        if (hashs[hash].hash == user_id)
        {
            return hashs[hash];
        }

        if (hashs[hash].hash == 0)
        {
            break; // Não encontrado
        }

        i++;
        hash = (hash1 + i * hash2) % HASH_SIZE;

        if (i >= HASH_SIZE)
        {
            break;
        }
    }

    MemoryIndex notFound = {0, -1};
    return notFound;
}

void find_evend_by_user_id(int user_id, MemoryIndex *hashs)
{
    MemoryIndex index = get_memory_index(user_id, hashs);
    if (index.hash == 0)
    {
        printf("Registro não encontrado para user_id: %d\n", user_id);
        return;
    }

    FILE *file = fopen(EVENT_FILE_NAME, "rb");
    if (file == NULL)
    {
        perror("Erro ao abrir o arquivo de eventos");
        return;
    }

    Event event;
    fseek(file, index.position * sizeof(Event), SEEK_SET);
    fread(&event, sizeof(Event), 1, file);

    printf("ID: %d\n", event.id);
    printf("Type: %s\n", event.event_type);
    printf("Product ID: %d\n", event.product_id);
    printf("User ID: %d\n", event.user_id);

    fclose(file);
}

void readAllHashs(MemoryIndex *hashs)
{
	int i;
    for (i = 0; i < HASH_SIZE; i++)
    {
        if (hashs[i].hash != 0)
        {
            printf("Hash: %d, Posicao: %d\n", hashs[i].hash, hashs[i].position);
        }
    }
}

int colisoes = 0;
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
    printf("8 -> Pesquisa SEQUENCIAL por ID em produtos (comparacao de tempo)\n");
    printf("9 -> Pesquisa Binaria (indice) e Sequencial por preco (comparacao de tempo)\n");
    printf("10 -> Criar arvore B de indice (id do produto)\n");
    printf("11 -> Comparar tempo de busca nos indices por id (produto)\n");
    printf("12 -> Criar indices por Hash\n");
    printf("13 -> Buscar Evento Por Hash de User Id\n");
    
    printf("0 -> Sair\n");
    printf("Opcao: ");
}

int main()
{
    int opc = 0;
    int numBlocks = 0;
    int idSearch = 0;
    float priceSearch;
    clock_t t;    
    int prod_pesquisa = 12719154; //100028530
    
    BTree* tree = create_btree();
    MemoryIndex *hashs = NULL; // Ponteiro para o Indice em memoria
    initialize_memory_index(&hashs);

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
            //printProductsFromFile(PROD_FILE_NAME);
            createEventFile(ORIGINAL_FILE_NAME);
            // printEventsFromFile(EVENT_FILE_NAME);
            break;
        case 2:
			t = clock();
            createProductsIndexFile(PROD_FILE_NAME);
            t = clock() - t;
            float tempoCriacao = (float)(t) / CLOCKS_PER_SEC;
            printIndexFile(PROD_IDINDEX_FILE_NAME);
            createEventsIndexFile(EVENT_FILE_NAME);
            // printIndexFile(EVENT_IDINDEX_FILE_NAME);
            printf("\nTempo para criacao do arquivo de indice por chave de ordenacao (produtos): %.4f\n", tempoCriacao);

            break;
        case 3:
            numBlocks = createTemporaryPriceIndexFiles(PROD_FILE_NAME);
            if (numBlocks > 0)
            {
                mergeSortedPriceBlocks(numBlocks);
                printf("Arquivo de indice ordenado por preco criado com sucesso.\n");
                // printPriceIndexFromFile(PROD_PRICE_INDEX_FILE_NAME);
            }
            else
            {
                printf("Erro ao criar os arquivos temporarios de indice.\n");
            }

			t = clock();
            numBlocks = createTemporaryUserIDIndexFiles(EVENT_FILE_NAME);
            if (numBlocks > 0)
            {
                mergeSortedUserIDBlocks(numBlocks);
                printf("Arquivo de indice ordenado por user_id criado com sucesso.\n");
                // printUserIDIndexFromFile(EVENT_USER_INDEX_FILE_NAME);
            }
            else
            {
                printf("Erro ao criar os arquivos temporarios de indice.\n");
            }

            t = clock() - t;
            float tempoCriacao2 = (float)(t) / CLOCKS_PER_SEC;
            printf("\nTempo para criacao do arquivo de indice por user_id (eventos): %.4f\n", tempoCriacao2);
            break;
        case 4:
            // printf("Digite o ID do produto: ");
            // scanf("%d", &idSearch);
            // binarySearchIndexProducts(idSearch);
            binarySearchIndexProducts(100028530);
            break;
        case 5:
            // printf("Digite o ID do evento: ");
            // scanf("%d", &idSearch);
            // binarySearchIndexEvents(idSearch);
            binarySearchIndexEvents(1205); // 1205 como exemplo
            break;
        case 6:
            // printf("Digite o preco do produto: ");
            // scanf("%f", &priceSearch);
            // binarySearchPriceProducts(priceSearch);
            binarySearchPriceProducts(5.77); // 5.77 como exemplo
            break;
        case 7:
            // printf("Digite o ID do usuario: ");
            // scanf("%d", &idSearch);
            // binarySearchUserIDEvents(idSearch);
            binarySearchUserIDEvents(518085591); // 518085591 como exemplo
            break;
        case 8:
        	t = clock();
            binarySearchIndexProducts(100028530);
            t = clock() - t;
            float tempoGastoBinario = (float)(t) / CLOCKS_PER_SEC;
        	t = clock();  		
    		printf("------------------------------\n");
            sequencialSearchProductsId(100028530); // ultimo id do arquivo
    		t = clock() - t;
    		float tempoGastoSequencial = (float)(t) / CLOCKS_PER_SEC;
    		printf("------------------------------\n");
    		printf("--- Comparacao de tempo ---\n");
    		printf("Busca binaria: %.4f s\n", tempoGastoBinario);
    		printf("Busca sequencial: %.4f s\n", tempoGastoSequencial);
            break;
        case 9:
        	t = clock();
            sequencialSearchProductsPrice(411.57);
    		t = clock() - t;
    		float tempoGastoSeqPrice = (float)(t) / CLOCKS_PER_SEC;
    		t = clock();
    		binarySearchPriceProducts(411.57);
    		t = clock() - t;    		
    		float tempoGastoBinPrice = (float)(t) / CLOCKS_PER_SEC;    		
    		printf("------------------------------\n");
    		printf("--- Comparacao de tempo ---\n");
    		printf("Busca binaria: %.4f s\n", tempoGastoBinPrice);
    		printf("Busca sequencial: %.4f s\n", tempoGastoSeqPrice);
            break;
        case 10:
        	t = clock();
            createBTreeIndexFile(PROD_FILE_NAME, tree);
    		t = clock() - t;
    		float tempoGastoCriarArvore = (float)(t) / CLOCKS_PER_SEC;
    		printf("Tempo gasto para criacao da arvore B: %.4f\n", tempoGastoCriarArvore);
        	break;
        case 11:
			t = clock();
            binarySearchIndexProducts(prod_pesquisa);
    		t = clock() - t;
    		float tempoGastoBin1 = (float)(t) / CLOCKS_PER_SEC;
    		t = clock();
    		search(tree, prod_pesquisa);
    		t = clock() - t;    		
    		float tempoGastoTree1 = (float)(t) / CLOCKS_PER_SEC;    
        	printf("------------------------------\n");
    		printf("--- Comparacao de tempo ---\n");
    		printf("Busca binaria (arquivo): %.4f s\n", tempoGastoBin1);
    		printf("Busca arvore B: %.4f s\n", tempoGastoTree1);
        case 12:
        	t = clock();
            createHashIndexes(EVENT_FILE_NAME, hashs);
    		t = clock() - t;
    		float tempoCriarHash = (float)(t) / CLOCKS_PER_SEC;
    		printf("Tempo gasto para criacao dos hashs: %.4f\n", tempoCriarHash);
    		printf("colisoes: %d\n", colisoes);
            colisoes = 0;
        	break;
        case 13:
            // printf("Digite o ID do usuario: ");
            // scanf("%d", &idSearch);
            // find_evend_by_user_id(idSearch);
            find_evend_by_user_id(517167420, hashs); // 541480181 como exemplo
            break;
        }

    } while (opc != 0);

    return 0;
}

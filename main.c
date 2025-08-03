#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "src/include/common_types.h"
#include "src/include/utils.h"

#include "src/include/sequencial.h" 
#include "src/include/arvBin.h"
#include "src/include/arvoreB.h"  
#include "src/include/arvoreEstrela.h"


int main(int argc, char *argv[]) {

    //entradas:
    int metodo_escolhido; //método de pesquisa
    long quantidade_alunos; //quantidade de lihas do arquivo a serem ordenadas
    int situacao_ordem; //situação de ordem inicial do arquivo
    bool exibir_dados_debug = false; // flag para exibir dados dos alunos e o resultado da ordanação

    // char filename[256];
    // FILE *arquivo_dados = NULL;

    // // Variáveis para medição de desempenho
    // clock_t inicio_tempo_geral;
    // double tempo_execucao_pesquisa;
    // double tempo_execucao_construcao_indice;

    // Verifica se o número mínimo de argumentos foi fornecido
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <metodo> <quantidade> <situacao> [-P]\n", argv[0]);
        fprintf(stderr, "  <metodo>: 1=Intercalação Balanceada com ordenação interna QuickSort, 2=Intercalação Balanceada com Seleção por Substituição, 3=QuickSort Externo\n");
        fprintf(stderr, "  <quantidade>: 100, 1000, 10000, 100000, 471705\n");
        fprintf(stderr, "  <situacao>: 1=Ascendente, 2=Descendente, 3=Aleatoria\n");
        fprintf(stderr, "  [-P]: Opcional, exibe dados dos alunos e o resultado da ordanação (debug)\n");
        return 1;
    }

    metodo_escolhido = atoi(argv[1]);
    quantidade_alunos = atol(argv[2]);
    situacao_ordem = atoi(argv[3]);

    //Verifica se as chaves de pesquisa dos registros devem ser impressos
    if (argc == 5 && strcmp(argv[4], "-P") == 0) {
        exibir_dados_debug = true;
    }

    //método inválido
    if (metodo_escolhido < 1 || metodo_escolhido > 3) {
        fprintf(stderr, "Erro: Metodo invalido. Escolha entre 1 e 3.\n");
        return 1;
    }
    //quantidade de registros inválida
    if (quantidade_registros != 100 && quantidade_registros != 1000 &&
        quantidade_registros != 10000 && quantidade_registros != 100000 &&
        quantidade_registros != 471705) {
        fprintf(stderr, "Erro: Quantidade de registros invalida. Escolha 100, 1000, 10000, 100000 ou 471705.\n");
        return 1;
    }
    //ordem inválida
    if (situacao_ordem < 1 || situacao_ordem > 3) {
        fprintf(stderr, "Erro: Situacao de ordem invalida. Escolha 1 (Ascendente), 2 (Descendente) ou 3 (Aleatoria).\n");
        return 1;
    }

    // Determina a string da situação para formar o nome do arquivo
    const char* situacao_str;
    if (situacao_ordem == 1) situacao_str = "asc";
    else if (situacao_ordem == 2) situacao_str = "desc";
    else situacao_str = "rand";
    
    sprintf(filename, "data/PROVAO_%ld_%s.txt", quantidade_registros, situacao_str);
     
    //abertura do arquivo para leitura
    arquivo = fopen(filename, "r");
    if (arquivo == NULL) {
        fprintf(stderr, "Erro: Nao foi possivel abrir o arquivo de dados '%s'. Certifique-se de que ele foi gerado.\n", filename);
        return 1;
    }

    printf("--- Iniciando Pesquisa ---\n");
    printf("Metodo: %d, Quantidade: %ld, Situacao: %d, Chave: %ld\n",
           metodo_escolhido, quantidade_registros, situacao_ordem, chave_pesquisa);
    printf("Arquivo de dados: %s\n", filename);

    TipoRegistro registro_encontrado;
    registro_encontrado.Chave = chave_pesquisa; // Define a chave no registro que será usado para a busca
    bool encontrado = false; // Flag para indicar se a chave foi encontrada

    switch (metodo_escolhido) {
        case 1: { // 1: Acesso Sequencial Indexado
            int tam_pagina_sequencial = 4;
            
            // Calcula o tamanho total do arquivo em número de registros
            fseek(arquivo_dados, 0, SEEK_END);
            long tam_arq_bytes = ftell(arquivo_dados);
            fseek(arquivo_dados, 0, SEEK_SET);

            int tam_arq_itens = tam_arq_bytes / sizeof(TipoRegistro);
            
            //Construção do Índice
            resetar_contadores();
            inicio_tempo_geral = iniciar_tempo();
            Indicie_p* indice_sequencial = criar_indicie_pagina(arquivo_dados, tam_arq_itens, tam_pagina_sequencial);
            tempo_execucao_construcao_indice = finalizar_tempo(inicio_tempo_geral);

            printf("\n--- Criacao do Indice (Sequencial Indexado) ---\n");
            printf("Tempo de construcao do indice: %.6f segundos\n", tempo_execucao_construcao_indice);
            printf("Transferencias (I/O) na construcao do indice: %ld\n", g_io_transferencias);
            printf("Comparacoes na construcao do indice: %ld\n", g_comparacoes_chaves);

            //pesquisa
            resetar_contadores();
            inicio_tempo_geral = iniciar_tempo();
            encontrado = pesquisa_sequencial(indice_sequencial, tam_pagina_sequencial, (tam_arq_itens + tam_pagina_sequencial - 1) / tam_pagina_sequencial, &registro_encontrado, arquivo_dados, situacao_ordem);
            
            tempo_execucao_pesquisa = finalizar_tempo(inicio_tempo_geral);
            free(indice_sequencial);
            break;
        }

        case 2: { // 2: Árvore Binária de Pesquisa 
            FILE* arv_bin_file = fopen("temp/arvore_binaria_externa.dat", "r+b");
            // Se o arquivo da árvore não existe, ele é criado
    
             arv_bin_file = fopen("temp/arvore_binaria_externa.dat", "w+b");
            if (!arv_bin_file) {
                perror("Erro ao criar/abrir arquivo da Arvore Binaria Externa");
                fclose(arquivo_dados);
                return 1;
            }

            //Construção da Árvore em Disco
            printf("\n--- Construcao da Arvore Binaria Externa (em disco) ---\n");
            resetar_contadores();
            inicio_tempo_geral = iniciar_tempo();
            
            TipoRegistro temp_reg;
            long pos_raiz_bin_arv = -1;
            fseek(arquivo_dados, 0, SEEK_SET);
            // Lê cada registro do arquivo de dados e insere na árvore em disco
            while(fread(&temp_reg, sizeof(TipoRegistro), 1, arquivo_dados) == 1) {
                incrementar_io();
                pos_raiz_bin_arv = inserir(arv_bin_file, temp_reg, pos_raiz_bin_arv);
            }
            tempo_execucao_construcao_indice = finalizar_tempo(inicio_tempo_geral);
            printf("Tempo de construcao do indice: %.6f segundos\n", tempo_execucao_construcao_indice);
            printf("Transferencias (I/O) na construcao do indice: %ld\n", g_io_transferencias);
            printf("Comparacoes na construcao do indice: %ld\n", g_comparacoes_chaves);
            

            //Pesquisa
            resetar_contadores();
            inicio_tempo_geral = iniciar_tempo();
            encontrado = buscar(arv_bin_file, chave_pesquisa, 0, &registro_encontrado);
            tempo_execucao_pesquisa = finalizar_tempo(inicio_tempo_geral);
            fclose(arv_bin_file);
            break;
        }

        case 3: { // 3: Árvore B
            Pagina* arvoreB_raiz = NULL;
            inicializa_arvoreB(&arvoreB_raiz); 

            //Construção da Árvore
            printf("\n--- Criacao do Indice (Arvore B) ---\n");
            resetar_contadores();
            inicio_tempo_geral = iniciar_tempo();
            
            TipoRegistro temp_reg_b;
            fseek(arquivo_dados, 0, SEEK_SET);
            // Lê cada registro e insere na Árvore B em memória
            while(fread(&temp_reg_b, sizeof(TipoRegistro), 1, arquivo_dados) == 1) {
                incrementar_io();
                insere_arvoreB(temp_reg_b, &arvoreB_raiz);
            }
            tempo_execucao_construcao_indice = finalizar_tempo(inicio_tempo_geral);
            printf("Tempo de construcao do indice: %.6f segundos\n", tempo_execucao_construcao_indice);
            printf("Transferencias (I/O) na construcao do indice: %ld\n", g_io_transferencias);
            printf("Comparacoes na construcao do indice: %ld\n", g_comparacoes_chaves);

            //Pesquisa
            resetar_contadores();
            inicio_tempo_geral = iniciar_tempo();
            encontrado = pesquisa_arvoreB(&registro_encontrado, arvoreB_raiz);
            tempo_execucao_pesquisa = finalizar_tempo(inicio_tempo_geral);
            
            libera_arvoreB(arvoreB_raiz);
            break;
        }

        case 4: { // 4: Árvore B*
            TipoApontador arvoreBStar_raiz = NULL;
            Inicializa(&arvoreBStar_raiz); 

            //Construção da Árvore
            printf("\n--- Criacao do Indice (Arvore B*) ---\n");
            resetar_contadores();
            inicio_tempo_geral = iniciar_tempo();
            
            TipoRegistro temp_reg_bstar;
            fseek(arquivo_dados, 0, SEEK_SET);
            // Lê cada registro e insere na Árvore B* em memória
            while(fread(&temp_reg_bstar, sizeof(TipoRegistro), 1, arquivo_dados) == 1) {
                incrementar_io();
                Insere(temp_reg_bstar, &arvoreBStar_raiz); // Insere espera TipoRegistro e TipoApontador*
            }
            tempo_execucao_construcao_indice = finalizar_tempo(inicio_tempo_geral);
            printf("Tempo de construcao do indice: %.6f segundos\n", tempo_execucao_construcao_indice);
            printf("Transferencias (I/O) na construcao do indice: %ld\n", g_io_transferencias);
            printf("Comparacoes na construcao do indice: %ld\n", g_comparacoes_chaves);

            //Pesquisa
            resetar_contadores();
            inicio_tempo_geral = iniciar_tempo();
            Pesquisa(&registro_encontrado, &arvoreBStar_raiz);
            if (registro_encontrado.Chave == chave_pesquisa) {
                encontrado = true;
            } else {
                encontrado = false;
            }
            
            tempo_execucao_pesquisa = finalizar_tempo(inicio_tempo_geral);
            liberaArvoreBStar(arvoreBStar_raiz);
            break;
        }
    }

    //Exibição dos Resultados Finais
    printf("\n--- Resultados da Pesquisa da Chave %ld ---\n", chave_pesquisa);
    if (encontrado) {
        printf("Chave %ld: ENCONTRADA!\n", chave_pesquisa);
        // printf("  Dado1: %ld\n", registro_encontrado.dado1);
        // printf("  Dado2: %s\n", registro_encontrado.dado2);
        // printf("  Dado3: %s\n", registro_encontrado.dado3);
    } else {
        printf("Chave %ld: NAO ENCONTRADA.\n", chave_pesquisa);
    }

    // Exibe as estatísticas de desempenho da fase de pesquisa
    printf("Numero de transferencias (I/O) na pesquisa: %ld\n", g_io_transferencias);
    printf("Numero de comparacoes entre chaves na pesquisa: %ld\n", g_comparacoes_chaves);
    printf("Tempo de execucao da pesquisa: %.6f segundos\n", tempo_execucao_pesquisa);

    //Seção de Debug -P
    if (exibir_chaves_debug) {
        printf("\n--- Conteudo do Arquivo (apenas chaves para debug) ---\n");
        fseek(arquivo_dados, 0, SEEK_SET);
        TipoRegistro temp_reg;
        int count = 0;
        while (fread(&temp_reg, sizeof(TipoRegistro), 1, arquivo_dados) == 1) {
            printf("%ld ", temp_reg.Chave);
            count++;
            if (count % 10 == 0) printf("\n");
        }
        printf("\n");
    }

    fclose(arquivo_dados);

    return 0;
}
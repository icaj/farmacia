#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARQ_USUARIOS "usuarios.dat"
#define ARQ_REMEDIOS "remedios.dat"
#define ARQ_LIXEIRA  "lixeira.dat"
#define ARQ_FILA     "fila.dat"

#define MAX_USUARIOS 10

typedef struct {
    char usuario[30];
    char senha[30];
} Usuario;

typedef struct Remedio {
    int id;
    char nome[80];
    char laboratorio[80];
    float preco;
    int quantidade;
    struct Remedio *prox;
} Remedio;

typedef struct FilaNo {
    int idRemedio;
    struct FilaNo *prox;
} FilaNo;

Remedio *lista = NULL;
Remedio *pilhaLixeira = NULL;

FilaNo *inicioFila = NULL;
FilaNo *fimFila = NULL;

/* ================= LOGIN ================= */

void criarUsuariosPadrao() {
    FILE *f = fopen(ARQ_USUARIOS, "rb");

    if (f != NULL) {
        fclose(f);
        return;
    }

    Usuario usuarios[2] = {
        {"admin", "123"},
        {"ivo", "123"}
    };

    f = fopen(ARQ_USUARIOS, "wb");
    fwrite(usuarios, sizeof(Usuario), 2, f);
    fclose(f);
}

int login() {
    Usuario usuarios[MAX_USUARIOS];
    Usuario temp;
    int qtd = 0;
    char user[30], senha[30];

    FILE *f = fopen(ARQ_USUARIOS, "rb");

    if (f == NULL) {
        printf("Erro ao abrir arquivo de usuarios.\n");
        return 0;
    }

    while (qtd < MAX_USUARIOS &&
           fread(&temp, sizeof(Usuario), 1, f) == 1) {
        usuarios[qtd++] = temp;
    }

    fclose(f);

    printf("Usuario: ");
    scanf("%29s", user);

    printf("Senha: ");
    scanf("%29s", senha);

    for (int i = 0; i < qtd; i++) {
        if (strcmp(user, usuarios[i].usuario) == 0 &&
            strcmp(senha, usuarios[i].senha) == 0) {
            return 1;
        }
    }

    return 0;
}

/* ================= UTIL ================= */

void limparBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

Remedio *criarNo(Remedio r) {
    Remedio *novo = malloc(sizeof(Remedio));

    if (novo == NULL) {
        printf("Erro de memoria.\n");
        exit(1);
    }

    *novo = r;
    novo->prox = NULL;

    return novo;
}

void inserirInicio(Remedio **cabeca, Remedio *novo) {
    novo->prox = *cabeca;
    *cabeca = novo;
}

Remedio *buscarPorId(Remedio *cabeca, int id) {
    while (cabeca != NULL) {
        if (cabeca->id == id) {
            return cabeca;
        }
        cabeca = cabeca->prox;
    }

    return NULL;
}

/* ================= CRUD ================= */

void cadastrarRemedio() {
    Remedio r;

    printf("ID: ");
    scanf("%d", &r.id);
    limparBuffer();

    if (buscarPorId(lista, r.id) != NULL) {
        printf("Ja existe remedio com este ID.\n");
        return;
    }

    printf("Nome: ");
    fgets(r.nome, sizeof(r.nome), stdin);
    r.nome[strcspn(r.nome, "\n")] = '\0';

    printf("Laboratorio: ");
    fgets(r.laboratorio, sizeof(r.laboratorio), stdin);
    r.laboratorio[strcspn(r.laboratorio, "\n")] = '\0';

    printf("Preco: ");
    scanf("%f", &r.preco);

    printf("Quantidade: ");
    scanf("%d", &r.quantidade);

    Remedio *novo = criarNo(r);
    inserirInicio(&lista, novo);

    printf("Remedio cadastrado com sucesso.\n");
}

void listarRemedios(Remedio *cabeca);

void listarLista(Remedio *cabeca);

void listarRemediosPrincipal(Remedio *cabeca);

void listarRemediosDaLista(Remedio *cabeca) {
    if (cabeca == NULL) {
        printf("Nenhum registro encontrado.\n");
        return;
    }

    while (cabeca != NULL) {
        printf("\nID: %d", cabeca->id);
        printf("\nNome: %s", cabeca->nome);
        printf("\nLaboratorio: %s", cabeca->laboratorio);
        printf("\nPreco: %.2f", cabeca->preco);
        printf("\nQuantidade: %d\n", cabeca->quantidade);

        cabeca = cabeca->prox;
    }
}

void editarRemedio() {
    int id;

    printf("Informe o ID do remedio: ");
    scanf("%d", &id);
    limparBuffer();

    Remedio *r = buscarPorId(lista, id);

    if (r == NULL) {
        printf("Remedio nao encontrado.\n");
        return;
    }

    printf("Novo nome: ");
    fgets(r->nome, sizeof(r->nome), stdin);
    r->nome[strcspn(r->nome, "\n")] = '\0';

    printf("Novo laboratorio: ");
    fgets(r->laboratorio, sizeof(r->laboratorio), stdin);
    r->laboratorio[strcspn(r->laboratorio, "\n")] = '\0';

    printf("Novo preco: ");
    scanf("%f", &r->preco);

    printf("Nova quantidade: ");
    scanf("%d", &r->quantidade);

    printf("Remedio alterado com sucesso.\n");
}

void excluirRemedio() {
    int id;
    Remedio *atual = lista;
    Remedio *anterior = NULL;

    printf("Informe o ID para excluir: ");
    scanf("%d", &id);

    while (atual != NULL && atual->id != id) {
        anterior = atual;
        atual = atual->prox;
    }

    if (atual == NULL) {
        printf("Remedio nao encontrado.\n");
        return;
    }

    if (anterior == NULL) {
        lista = atual->prox;
    } else {
        anterior->prox = atual->prox;
    }

    atual->prox = NULL;
    inserirInicio(&pilhaLixeira, atual);

    printf("Remedio movido para a lixeira.\n");
}

void desfazerExclusao() {
    if (pilhaLixeira == NULL) {
        printf("Lixeira vazia.\n");
        return;
    }

    Remedio *restaurado = pilhaLixeira;
    pilhaLixeira = pilhaLixeira->prox;

    restaurado->prox = NULL;
    inserirInicio(&lista, restaurado);

    printf("Ultima exclusao desfeita.\n");
}

/* ================= PESQUISA ================= */

void pesquisarSequencialNome() {
    char nome[80];
    int achou = 0;

    limparBuffer();

    printf("Digite o nome para pesquisar: ");
    fgets(nome, sizeof(nome), stdin);
    nome[strcspn(nome, "\n")] = '\0';

    Remedio *atual = lista;

    while (atual != NULL) {
        if (strstr(atual->nome, nome) != NULL) {
            printf("\nID: %d", atual->id);
            printf("\nNome: %s", atual->nome);
            printf("\nPreco: %.2f\n", atual->preco);
            achou = 1;
        }

        atual = atual->prox;
    }

    if (!achou) {
        printf("Nenhum remedio encontrado.\n");
    }
}

/* ================= ORDENAÇÃO E BUSCA BINÁRIA ================= */

int contarLista(Remedio *cabeca);

int contarRemedios(Remedio *cabeca) {
    int qtd = 0;

    while (cabeca != NULL) {
        qtd++;
        cabeca = cabeca->prox;
    }

    return qtd;
}

int compararPorNome(const void *a, const void *b) {
    Remedio *r1 = *(Remedio **)a;
    Remedio *r2 = *(Remedio **)b;

    return strcmp(r1->nome, r2->nome);
}

int compararPorId(const void *a, const void *b) {
    Remedio *r1 = *(Remedio **)a;
    Remedio *r2 = *(Remedio **)b;

    return r1->id - r2->id;
}

void ordenarPorNome() {
    int qtd = contarRemedios(lista);

    if (qtd == 0) {
        printf("Lista vazia.\n");
        return;
    }

    Remedio **vetor = malloc(qtd * sizeof(Remedio *));

    Remedio *atual = lista;
    for (int i = 0; i < qtd; i++) {
        vetor[i] = atual;
        atual = atual->prox;
    }

    qsort(vetor, qtd, sizeof(Remedio *), compararPorNome);

    printf("\n=== REMEDIOS ORDENADOS POR NOME ===\n");

    for (int i = 0; i < qtd; i++) {
        printf("\nID: %d", vetor[i]->id);
        printf("\nNome: %s", vetor[i]->nome);
        printf("\nPreco: %.2f\n", vetor[i]->preco);
    }

    free(vetor);
}

void buscaBinariaPorId() {
    int qtd = contarRemedios(lista);
    int id;

    if (qtd == 0) {
        printf("Lista vazia.\n");
        return;
    }

    Remedio **vetor = malloc(qtd * sizeof(Remedio *));

    Remedio *atual = lista;
    for (int i = 0; i < qtd; i++) {
        vetor[i] = atual;
        atual = atual->prox;
    }

    qsort(vetor, qtd, sizeof(Remedio *), compararPorId);

    printf("Informe o ID: ");
    scanf("%d", &id);

    int ini = 0;
    int fim = qtd - 1;
    int achou = 0;

    while (ini <= fim) {
        int meio = (ini + fim) / 2;

        if (vetor[meio]->id == id) {
            printf("\nRemedio encontrado:");
            printf("\nID: %d", vetor[meio]->id);
            printf("\nNome: %s", vetor[meio]->nome);
            printf("\nPreco: %.2f\n", vetor[meio]->preco);
            achou = 1;
            break;
        } else if (id < vetor[meio]->id) {
            fim = meio - 1;
        } else {
            ini = meio + 1;
        }
    }

    if (!achou) {
        printf("Remedio nao encontrado.\n");
    }

    free(vetor);
}

/* ================= FILA ================= */

void adicionarFila() {
    int id;

    printf("Informe o ID do remedio pendente: ");
    scanf("%d", &id);

    if (buscarPorId(lista, id) == NULL) {
        printf("ID nao encontrado na lista principal.\n");
        return;
    }

    FilaNo *novo = malloc(sizeof(FilaNo));
    novo->idRemedio = id;
    novo->prox = NULL;

    if (fimFila == NULL) {
        inicioFila = novo;
        fimFila = novo;
    } else {
        fimFila->prox = novo;
        fimFila = novo;
    }

    printf("Item adicionado na fila.\n");
}

void removerFila() {
    if (inicioFila == NULL) {
        printf("Fila vazia.\n");
        return;
    }

    FilaNo *remover = inicioFila;

    printf("Removido da fila o ID: %d\n", remover->idRemedio);

    inicioFila = inicioFila->prox;

    if (inicioFila == NULL) {
        fimFila = NULL;
    }

    free(remover);
}

void listarFila() {
    FilaNo *atual = inicioFila;

    if (atual == NULL) {
        printf("Fila vazia.\n");
        return;
    }

    printf("\n=== FILA DE PENDENTES ===\n");

    while (atual != NULL) {
        printf("ID remedio: %d\n", atual->idRemedio);
        atual = atual->prox;
    }
}

/* ================= PERSISTÊNCIA ================= */

void salvarListaArquivo(char *nomeArquivo, Remedio *cabeca) {
    FILE *f = fopen(nomeArquivo, "wb");

    if (f == NULL) {
        printf("Erro ao salvar %s.\n", nomeArquivo);
        return;
    }

    while (cabeca != NULL) {
        fwrite(cabeca, sizeof(Remedio), 1, f);
        cabeca = cabeca->prox;
    }

    fclose(f);
}

void carregarListaArquivo(char *nomeArquivo, Remedio **cabeca) {
    FILE *f = fopen(nomeArquivo, "rb");

    if (f == NULL) {
        return;
    }

    Remedio temp;

    while (fread(&temp, sizeof(Remedio), 1, f) == 1) {
        temp.prox = NULL;
        Remedio *novo = criarNo(temp);
        inserirInicio(cabeca, novo);
    }

    fclose(f);
}

void salvarFila() {
    FILE *f = fopen(ARQ_FILA, "wb");

    if (f == NULL) {
        printf("Erro ao salvar fila.\n");
        return;
    }

    FilaNo *atual = inicioFila;

    while (atual != NULL) {
        fwrite(&atual->idRemedio, sizeof(int), 1, f);
        atual = atual->prox;
    }

    fclose(f);
}

void carregarFila() {
    FILE *f = fopen(ARQ_FILA, "rb");

    if (f == NULL) {
        return;
    }

    int id;

    while (fread(&id, sizeof(int), 1, f) == 1) {
        FilaNo *novo = malloc(sizeof(FilaNo));
        novo->idRemedio = id;
        novo->prox = NULL;

        if (fimFila == NULL) {
            inicioFila = novo;
            fimFila = novo;
        } else {
            fimFila->prox = novo;
            fimFila = novo;
        }
    }

    fclose(f);
}

void salvarTudo() {
    salvarListaArquivo(ARQ_REMEDIOS, lista);
    salvarListaArquivo(ARQ_LIXEIRA, pilhaLixeira);
    salvarFila();

    printf("Dados salvos com sucesso.\n");
}

void carregarTudo() {
    carregarListaArquivo(ARQ_REMEDIOS, &lista);
    carregarListaArquivo(ARQ_LIXEIRA, &pilhaLixeira);
    carregarFila();
}

/* ================= MENU ================= */

void menu() {
    int op;

    do {
        printf("\n===== SISTEMA FARMACIA =====\n");
        printf("1 - Cadastrar remedio\n");
        printf("2 - Listar remedios\n");
        printf("3 - Editar remedio\n");
        printf("4 - Excluir remedio\n");
        printf("5 - Desfazer ultima exclusao\n");
        printf("6 - Pesquisar por nome\n");
        printf("7 - Ordenar por nome\n");
        printf("8 - Busca binaria por ID\n");
        printf("9 - Adicionar item na fila\n");
        printf("10 - Remover item da fila\n");
        printf("11 - Listar fila\n");
        printf("12 - Listar lixeira\n");
        printf("0 - Salvar e sair\n");
        printf("Escolha: ");
        scanf("%d", &op);

        switch (op) {
            case 1:
                cadastrarRemedio();
                break;
            case 2:
                listarRemediosDaLista(lista);
                break;
            case 3:
                editarRemedio();
                break;
            case 4:
                excluirRemedio();
                break;
            case 5:
                desfazerExclusao();
                break;
            case 6:
                pesquisarSequencialNome();
                break;
            case 7:
                ordenarPorNome();
                break;
            case 8:
                buscaBinariaPorId();
                break;
            case 9:
                adicionarFila();
                break;
            case 10:
                removerFila();
                break;
            case 11:
                listarFila();
                break;
            case 12:
                listarRemediosDaLista(pilhaLixeira);
                break;
            case 0:
                salvarTudo();
                printf("Saindo...\n");
                break;
            default:
                printf("Opcao invalida.\n");
        }

    } while (op != 0);
}

/* ================= MAIN ================= */

int main() {
    criarUsuariosPadrao();

    printf("===== LOGIN =====\n");

    if (!login()) {
        printf("Usuario ou senha invalidos.\n");
        return 0;
    }

    printf("Login realizado com sucesso.\n");

    carregarTudo();

    menu();

    return 0;
}
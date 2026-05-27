/*
    Sistema de Cadastro de Remedios - Farmacia
    Linguagem C + ncurses/PDCurses

    Recursos implementados:
    - Login com usuarios persistidos em arquivo
    - Cadastro principal usando Lista Encadeada
    - CRUD de remedios
    - Pesquisa sequencial por nome
    - Ordenacao por nome usando qsort sobre vetor de ponteiros
    - Busca binaria por ID usando vetor temporario ordenado
    - Lixeira usando Pilha
    - Fila de itens pendentes
    - Persistencia em arquivos binarios
    - Telas coloridas em modo texto com curses

    Compilacao Linux:
        gcc main.c -o farmacia -lncurses

    Compilacao Windows com PDCurses:
        gcc main.c -o farmacia.exe -lpdcurses
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>

#define ARQ_USUARIOS "usuarios.dat"
#define ARQ_REMEDIOS "remedios.dat"
#define ARQ_LIXEIRA  "lixeira.dat"
#define ARQ_FILA     "fila.dat"

#define MAX_USUARIOS 10

#define COR_TITULO     1
#define COR_MENU       2
#define COR_CAMPO      3
#define COR_SUCESSO    4
#define COR_ERRO       5
#define COR_RODAPE     6
#define COR_CAIXA      7
#define COR_DESTAQUE   8

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
Usuario usuarioLogado;

FilaNo *inicioFila = NULL;
FilaNo *fimFila = NULL;

/* ============================================================
   FUNCOES DE TELA
   ============================================================ */

void pausar() {
    attron(COLOR_PAIR(COR_RODAPE));
    mvhline(LINES - 2, 1, ' ', COLS - 2);
    mvprintw(LINES - 2, 3, " Pressione qualquer tecla para continuar ");
    attroff(COLOR_PAIR(COR_RODAPE));
    refresh();
    getch();
}

void iniciarCores() {
    if (has_colors()) {
        start_color();
        use_default_colors();

        init_pair(COR_TITULO,   COLOR_WHITE,  COLOR_BLUE);
        init_pair(COR_MENU,     COLOR_CYAN,   -1);
        init_pair(COR_CAMPO,    COLOR_YELLOW, -1);
        init_pair(COR_SUCESSO,  COLOR_GREEN,  -1);
        init_pair(COR_ERRO,     COLOR_RED,    -1);
        init_pair(COR_RODAPE,   COLOR_BLACK,  COLOR_CYAN);
        init_pair(COR_CAIXA,    COLOR_WHITE,  -1);
        init_pair(COR_DESTAQUE, COLOR_BLACK,  COLOR_YELLOW);
    }
}

void desenharTelaBase(const char *titulo) {
    clear();

    attron(COLOR_PAIR(COR_CAIXA));
    box(stdscr, 0, 0);
    attroff(COLOR_PAIR(COR_CAIXA));

    attron(COLOR_PAIR(COR_TITULO) | A_BOLD);
    mvhline(1, 1, ' ', COLS - 2);
    mvprintw(1, 3, " %s | Usuario: %s", titulo, usuarioLogado.usuario);
    attroff(COLOR_PAIR(COR_TITULO) | A_BOLD);

    attron(COLOR_PAIR(COR_RODAPE));
    mvhline(LINES - 2, 1, ' ', COLS - 2);
    mvprintw(LINES - 2, 3, " 0 - Sair/Voltar | ENTER - Confirmar ");
    attroff(COLOR_PAIR(COR_RODAPE));

    refresh();
}

void imprimirCampo(int linha, int coluna, const char *label) {
    attron(COLOR_PAIR(COR_CAMPO) | A_BOLD);
    mvprintw(linha, coluna, "%s", label);
    attroff(COLOR_PAIR(COR_CAMPO) | A_BOLD);
}

void mensagemSucesso(const char *msg) {
    attron(COLOR_PAIR(COR_SUCESSO) | A_BOLD);
    mvprintw(LINES - 4, 3, "%s", msg);
    attroff(COLOR_PAIR(COR_SUCESSO) | A_BOLD);
    pausar();
}

void mensagemErro(const char *msg) {
    attron(COLOR_PAIR(COR_ERRO) | A_BOLD);
    mvprintw(LINES - 4, 3, "%s", msg);
    attroff(COLOR_PAIR(COR_ERRO) | A_BOLD);
    pausar();
}

void lerString(int linha, int coluna, char *destino, int tamanho) {
    echo();
    curs_set(1);
    move(linha, coluna);
    getnstr(destino, tamanho - 1);
    noecho();
    curs_set(0);
}

int lerInteiro(int linha, int coluna) {
    int valor;
    echo();
    curs_set(1);
    move(linha, coluna);
    scanw("%d", &valor);
    noecho();
    curs_set(0);
    return valor;
}

float lerFloat(int linha, int coluna) {
    float valor;
    echo();
    curs_set(1);
    move(linha, coluna);
    scanw("%f", &valor);
    noecho();
    curs_set(0);
    return valor;
}

/* ============================================================
   LOGIN
   ============================================================ */

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

    if (f != NULL) {
        fwrite(usuarios, sizeof(Usuario), 2, f);
        fclose(f);
    }
}

int telaLogin() {
    Usuario u;
    char usuario[30];
    char senha[30];

    desenharTelaBase("LOGIN DO SISTEMA");

    attron(COLOR_PAIR(COR_MENU) | A_BOLD);
    mvprintw(3, 5, "Acesso restrito ao sistema da farmacia");
    attroff(COLOR_PAIR(COR_MENU) | A_BOLD);

    imprimirCampo(6, 5, "Usuario: ");
    lerString(6, 15, usuario, sizeof(usuario));

    imprimirCampo(7, 5, "Senha: ");
    noecho();
    curs_set(1);
    move(7, 15);
    getnstr(senha, sizeof(senha) - 1);
    curs_set(0);

    FILE *f = fopen(ARQ_USUARIOS, "rb");

    if (f == NULL) {
        return 0;
    }

    while (fread(&u, sizeof(Usuario), 1, f) == 1) {
        if (strcmp(usuario, u.usuario) == 0 &&
            strcmp(senha, u.senha) == 0) {
            usuarioLogado = u;
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

/* ============================================================
   LISTA ENCADEADA
   ============================================================ */

Remedio *criarNo(Remedio r) {
    Remedio *novo = (Remedio *) malloc(sizeof(Remedio));

    if (novo == NULL) {
        endwin();
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

int contarRemedios(Remedio *cabeca) {
    int qtd = 0;

    while (cabeca != NULL) {
        qtd++;
        cabeca = cabeca->prox;
    }

    return qtd;
}

/* ============================================================
   CRUD
   ============================================================ */

void cadastrarRemedio() {
    Remedio r;

    desenharTelaBase("CADASTRO DE REMEDIO");

    imprimirCampo(4, 5, "ID: ");
    r.id = lerInteiro(4, 25);

    if (buscarPorId(lista, r.id) != NULL) {
        mensagemErro("Ja existe remedio com este ID.");
        return;
    }

    imprimirCampo(5, 5, "Nome: ");
    lerString(5, 25, r.nome, sizeof(r.nome));

    imprimirCampo(6, 5, "Laboratorio: ");
    lerString(6, 25, r.laboratorio, sizeof(r.laboratorio));

    imprimirCampo(7, 5, "Preco: ");
    r.preco = lerFloat(7, 25);

    imprimirCampo(8, 5, "Quantidade: ");
    r.quantidade = lerInteiro(8, 25);

    inserirInicio(&lista, criarNo(r));

    mensagemSucesso("Remedio cadastrado com sucesso.");
}

void listarRemedios(Remedio *cabeca, const char *titulo) {
    int linha = 5;

    desenharTelaBase(titulo);

    if (cabeca == NULL) {
        mensagemErro("Nenhum registro encontrado.");
        return;
    }

    attron(COLOR_PAIR(COR_TITULO) | A_BOLD);
    mvprintw(3, 4, "%-5s %-25s %-20s %-12s %-8s",
             "ID", "NOME", "LABORATORIO", "PRECO", "QTD");
    attroff(COLOR_PAIR(COR_TITULO) | A_BOLD);

    while (cabeca != NULL) {
        mvprintw(linha, 4, "%-5d %-25.25s %-20.20s R$ %-9.2f %-8d",
                 cabeca->id,
                 cabeca->nome,
                 cabeca->laboratorio,
                 cabeca->preco,
                 cabeca->quantidade);

        linha++;

        if (linha > LINES - 5) {
            pausar();
            desenharTelaBase(titulo);

            attron(COLOR_PAIR(COR_TITULO) | A_BOLD);
            mvprintw(3, 4, "%-5s %-25s %-20s %-12s %-8s",
                     "ID", "NOME", "LABORATORIO", "PRECO", "QTD");
            attroff(COLOR_PAIR(COR_TITULO) | A_BOLD);

            linha = 5;
        }

        cabeca = cabeca->prox;
    }

    pausar();
}

void editarRemedio() {
    int id;

    desenharTelaBase("EDITAR REMEDIO");

    imprimirCampo(4, 5, "Informe o ID: ");
    id = lerInteiro(4, 25);

    Remedio *r = buscarPorId(lista, id);

    if (r == NULL) {
        mensagemErro("Remedio nao encontrado.");
        return;
    }

    attron(COLOR_PAIR(COR_DESTAQUE) | A_BOLD);
    mvprintw(6, 5, " Editando: %s ", r->nome);
    attroff(COLOR_PAIR(COR_DESTAQUE) | A_BOLD);

    imprimirCampo(8, 5, "Novo nome: ");
    lerString(8, 25, r->nome, sizeof(r->nome));

    imprimirCampo(9, 5, "Novo laboratorio: ");
    lerString(9, 25, r->laboratorio, sizeof(r->laboratorio));

    imprimirCampo(10, 5, "Novo preco: ");
    r->preco = lerFloat(10, 25);

    imprimirCampo(11, 5, "Nova quantidade: ");
    r->quantidade = lerInteiro(11, 25);

    mensagemSucesso("Remedio alterado com sucesso.");
}

void excluirRemedio() {
    int id;
    Remedio *atual = lista;
    Remedio *anterior = NULL;

    desenharTelaBase("EXCLUIR REMEDIO");

    imprimirCampo(4, 5, "Informe o ID: ");
    id = lerInteiro(4, 25);

    while (atual != NULL && atual->id != id) {
        anterior = atual;
        atual = atual->prox;
    }

    if (atual == NULL) {
        mensagemErro("Remedio nao encontrado.");
        return;
    }

    if (anterior == NULL) {
        lista = atual->prox;
    } else {
        anterior->prox = atual->prox;
    }

    atual->prox = NULL;
    inserirInicio(&pilhaLixeira, atual);

    mensagemSucesso("Remedio movido para a lixeira.");
}

void desfazerExclusao() {
    Remedio *restaurado;

    desenharTelaBase("DESFAZER ULTIMA EXCLUSAO");

    if (pilhaLixeira == NULL) {
        mensagemErro("Lixeira vazia.");
        return;
    }

    restaurado = pilhaLixeira;
    pilhaLixeira = pilhaLixeira->prox;

    restaurado->prox = NULL;
    inserirInicio(&lista, restaurado);

    mensagemSucesso("Ultima exclusao desfeita com sucesso.");
}

/* ============================================================
   PESQUISA, ORDENACAO E BUSCA BINARIA
   ============================================================ */

void pesquisarPorNome() {
    char nome[80];
    Remedio *atual = lista;
    int linha = 7;
    int encontrou = 0;

    desenharTelaBase("PESQUISAR REMEDIO POR NOME");

    imprimirCampo(4, 5, "Nome: ");
    lerString(4, 15, nome, sizeof(nome));

    attron(COLOR_PAIR(COR_TITULO) | A_BOLD);
    mvprintw(6, 4, "%-5s %-25s %-12s", "ID", "NOME", "PRECO");
    attroff(COLOR_PAIR(COR_TITULO) | A_BOLD);

    while (atual != NULL) {
        if (strstr(atual->nome, nome) != NULL) {
            mvprintw(linha++, 4, "%-5d %-25.25s R$ %-9.2f",
                     atual->id, atual->nome, atual->preco);
            encontrou = 1;
        }

        atual = atual->prox;
    }

    if (!encontrou) {
        mensagemErro("Nenhum remedio encontrado.");
        return;
    }

    pausar();
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
    int linha = 5;

    desenharTelaBase("REMEDIOS ORDENADOS POR NOME");

    if (qtd == 0) {
        mensagemErro("Lista vazia.");
        return;
    }

    Remedio **vetor = (Remedio **) malloc(qtd * sizeof(Remedio *));

    if (vetor == NULL) {
        mensagemErro("Erro de memoria.");
        return;
    }

    Remedio *atual = lista;

    for (int i = 0; i < qtd; i++) {
        vetor[i] = atual;
        atual = atual->prox;
    }

    qsort(vetor, qtd, sizeof(Remedio *), compararPorNome);

    attron(COLOR_PAIR(COR_TITULO) | A_BOLD);
    mvprintw(3, 4, "%-5s %-25s %-12s", "ID", "NOME", "PRECO");
    attroff(COLOR_PAIR(COR_TITULO) | A_BOLD);

    for (int i = 0; i < qtd; i++) {
        mvprintw(linha++, 4, "%-5d %-25.25s R$ %-9.2f",
                 vetor[i]->id,
                 vetor[i]->nome,
                 vetor[i]->preco);

        if (linha > LINES - 5) {
            pausar();
            desenharTelaBase("REMEDIOS ORDENADOS POR NOME");
            linha = 5;
        }
    }

    free(vetor);
    pausar();
}

void buscaBinariaPorId() {
    int qtd = contarRemedios(lista);
    int id;
    int ini;
    int fim;
    int encontrou = 0;

    desenharTelaBase("BUSCA BINARIA POR ID");

    if (qtd == 0) {
        mensagemErro("Lista vazia.");
        return;
    }

    Remedio **vetor = (Remedio **) malloc(qtd * sizeof(Remedio *));

    if (vetor == NULL) {
        mensagemErro("Erro de memoria.");
        return;
    }

    Remedio *atual = lista;

    for (int i = 0; i < qtd; i++) {
        vetor[i] = atual;
        atual = atual->prox;
    }

    qsort(vetor, qtd, sizeof(Remedio *), compararPorId);

    imprimirCampo(4, 5, "Informe o ID: ");
    id = lerInteiro(4, 25);

    ini = 0;
    fim = qtd - 1;

    while (ini <= fim) {
        int meio = (ini + fim) / 2;

        if (vetor[meio]->id == id) {
            attron(COLOR_PAIR(COR_SUCESSO) | A_BOLD);
            mvprintw(6, 5, "Remedio encontrado:");
            attroff(COLOR_PAIR(COR_SUCESSO) | A_BOLD);

            mvprintw(8, 5, "ID: %d", vetor[meio]->id);
            mvprintw(9, 5, "Nome: %s", vetor[meio]->nome);
            mvprintw(10, 5, "Laboratorio: %s", vetor[meio]->laboratorio);
            mvprintw(11, 5, "Preco: %.2f", vetor[meio]->preco);
            mvprintw(12, 5, "Quantidade: %d", vetor[meio]->quantidade);

            encontrou = 1;
            break;
        } else if (id < vetor[meio]->id) {
            fim = meio - 1;
        } else {
            ini = meio + 1;
        }
    }

    free(vetor);

    if (!encontrou) {
        mensagemErro("Remedio nao encontrado.");
        return;
    }

    pausar();
}

/* ============================================================
   FILA
   ============================================================ */

void adicionarFila() {
    int id;

    desenharTelaBase("ADICIONAR ITEM NA FILA");

    imprimirCampo(4, 5, "ID do remedio: ");
    id = lerInteiro(4, 25);

    if (buscarPorId(lista, id) == NULL) {
        mensagemErro("ID nao encontrado na lista principal.");
        return;
    }

    FilaNo *novo = (FilaNo *) malloc(sizeof(FilaNo));

    if (novo == NULL) {
        mensagemErro("Erro de memoria.");
        return;
    }

    novo->idRemedio = id;
    novo->prox = NULL;

    if (fimFila == NULL) {
        inicioFila = novo;
        fimFila = novo;
    } else {
        fimFila->prox = novo;
        fimFila = novo;
    }

    mensagemSucesso("Item adicionado na fila.");
}

void removerFila() {
    desenharTelaBase("REMOVER ITEM DA FILA");

    if (inicioFila == NULL) {
        mensagemErro("Fila vazia.");
        return;
    }

    FilaNo *remover = inicioFila;

    attron(COLOR_PAIR(COR_SUCESSO) | A_BOLD);
    mvprintw(5, 5, "Removido da fila o ID: %d", remover->idRemedio);
    attroff(COLOR_PAIR(COR_SUCESSO) | A_BOLD);

    inicioFila = inicioFila->prox;

    if (inicioFila == NULL) {
        fimFila = NULL;
    }

    free(remover);
    pausar();
}

void listarFila() {
    FilaNo *atual = inicioFila;
    int linha = 5;
    int posicao = 1;

    desenharTelaBase("FILA DE PENDENTES");

    if (atual == NULL) {
        mensagemErro("Fila vazia.");
        return;
    }

    attron(COLOR_PAIR(COR_TITULO) | A_BOLD);
    mvprintw(3, 5, "%-10s %-15s", "POSICAO", "ID REMEDIO");
    attroff(COLOR_PAIR(COR_TITULO) | A_BOLD);

    while (atual != NULL) {
        mvprintw(linha++, 5, "%-10d %-15d", posicao, atual->idRemedio);
        posicao++;
        atual = atual->prox;
    }

    pausar();
}

/* ============================================================
   PERSISTENCIA
   ============================================================ */

void salvarListaArquivo(const char *nomeArquivo, Remedio *cabeca) {
    FILE *f = fopen(nomeArquivo, "wb");

    if (f == NULL) {
        return;
    }

    while (cabeca != NULL) {
        fwrite(cabeca, sizeof(Remedio), 1, f);
        cabeca = cabeca->prox;
    }

    fclose(f);
}

void carregarListaArquivo(const char *nomeArquivo, Remedio **cabeca) {
    FILE *f = fopen(nomeArquivo, "rb");
    Remedio temp;

    if (f == NULL) {
        return;
    }

    while (fread(&temp, sizeof(Remedio), 1, f) == 1) {
        temp.prox = NULL;
        inserirInicio(cabeca, criarNo(temp));
    }

    fclose(f);
}

void salvarFila() {
    FILE *f = fopen(ARQ_FILA, "wb");

    if (f == NULL) {
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
    int id;

    if (f == NULL) {
        return;
    }

    while (fread(&id, sizeof(int), 1, f) == 1) {
        FilaNo *novo = (FilaNo *) malloc(sizeof(FilaNo));

        if (novo == NULL) {
            fclose(f);
            return;
        }

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
}

void carregarTudo() {
    carregarListaArquivo(ARQ_REMEDIOS, &lista);
    carregarListaArquivo(ARQ_LIXEIRA, &pilhaLixeira);
    carregarFila();
}

/* ============================================================
   LIBERACAO DE MEMORIA
   ============================================================ */

void liberarLista(Remedio *cabeca) {
    Remedio *aux;

    while (cabeca != NULL) {
        aux = cabeca;
        cabeca = cabeca->prox;
        free(aux);
    }
}

void liberarFila() {
    FilaNo *aux;

    while (inicioFila != NULL) {
        aux = inicioFila;
        inicioFila = inicioFila->prox;
        free(aux);
    }

    fimFila = NULL;
}

/* ============================================================
   MENU
   ============================================================ */

int menuPrincipal() {
    int opcao;

    desenharTelaBase("SISTEMA DE FARMACIA - MENU PRINCIPAL");

    attron(COLOR_PAIR(COR_MENU) | A_BOLD);

    mvprintw(4, 6,  "1  - Cadastrar remedio");
    mvprintw(5, 6,  "2  - Listar remedios");
    mvprintw(6, 6,  "3  - Editar remedio");
    mvprintw(7, 6,  "4  - Excluir remedio");
    mvprintw(8, 6,  "5  - Desfazer ultima exclusao");
    mvprintw(9, 6,  "6  - Pesquisar por nome");
    mvprintw(10, 6, "7  - Ordenar por nome");
    mvprintw(11, 6, "8  - Busca binaria por ID");
    mvprintw(12, 6, "9  - Adicionar item na fila");
    mvprintw(13, 6, "10 - Remover item da fila");
    mvprintw(14, 6, "11 - Listar fila");
    mvprintw(15, 6, "12 - Listar lixeira");
    mvprintw(16, 6, "0  - Salvar e sair");

    attroff(COLOR_PAIR(COR_MENU) | A_BOLD);

    imprimirCampo(19, 6, "Escolha uma opcao: ");
    opcao = lerInteiro(19, 26);

    return opcao;
}

/* ============================================================
   MAIN
   ============================================================ */

int main() {
    int opcao;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    iniciarCores();

    criarUsuariosPadrao();

    if (!telaLogin()) {
        desenharTelaBase("LOGIN DO SISTEMA");
        mensagemErro("Usuario ou senha invalidos.");
        endwin();
        return 0;
    }

    carregarTudo();

    do {
        opcao = menuPrincipal();

        switch (opcao) {
            case 1:
                cadastrarRemedio();
                break;
            case 2:
                listarRemedios(lista, "LISTAGEM DE REMEDIOS");
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
                pesquisarPorNome();
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
                listarRemedios(pilhaLixeira, "LIXEIRA DE REMEDIOS");
                break;
            case 0:
                salvarTudo();
                break;
            default:
                mensagemErro("Opcao invalida.");
        }

    } while (opcao != 0);

    liberarLista(lista);
    liberarLista(pilhaLixeira);
    liberarFila();

    endwin();

    printf("Dados salvos com sucesso.\n");

    return 0;
}

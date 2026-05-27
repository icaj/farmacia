#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>

#define ARQ_REMEDIOS "remedios.dat"
#define ARQ_USUARIOS "usuarios.dat"

typedef struct Remedio {
    int id;
    char nome[80];
    char laboratorio[80];
    float preco;
    int quantidade;
    struct Remedio *prox;
} Remedio;

typedef struct {
    char usuario[30];
    char senha[30];
} Usuario;

Remedio *lista = NULL;
Remedio *lixeira = NULL;

void pausar() {
    mvprintw(LINES - 2, 2, "Pressione qualquer tecla para continuar...");
    getch();
}

void limparTela() {
    clear();
    box(stdscr, 0, 0);
}

void mensagem(const char *msg) {
    mvprintw(LINES - 3, 2, "%s", msg);
    pausar();
}

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

int telaLogin() {
    Usuario u;
    char usuario[30], senha[30];
    FILE *f;

    limparTela();
    mvprintw(2, 4, "LOGIN DO SISTEMA");
    mvprintw(4, 4, "Usuario: ");
    echo();
    getnstr(usuario, 29);

    mvprintw(5, 4, "Senha: ");
    noecho();
    getnstr(senha, 29);

    f = fopen(ARQ_USUARIOS, "rb");

    if (f == NULL) {
        return 0;
    }

    while (fread(&u, sizeof(Usuario), 1, f) == 1) {
        if (strcmp(usuario, u.usuario) == 0 &&
            strcmp(senha, u.senha) == 0) {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

Remedio *criarNo(Remedio r) {
    Remedio *novo = malloc(sizeof(Remedio));

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

void cadastrarRemedio() {
    Remedio r;

    limparTela();

    mvprintw(2, 4, "CADASTRO DE REMEDIO");

    echo();

    mvprintw(4, 4, "ID: ");
    scanw("%d", &r.id);

    if (buscarPorId(lista, r.id) != NULL) {
        noecho();
        mensagem("Ja existe remedio com este ID.");
        return;
    }

    mvprintw(5, 4, "Nome: ");
    getnstr(r.nome, 79);

    mvprintw(6, 4, "Laboratorio: ");
    getnstr(r.laboratorio, 79);

    mvprintw(7, 4, "Preco: ");
    scanw("%f", &r.preco);

    mvprintw(8, 4, "Quantidade: ");
    scanw("%d", &r.quantidade);

    noecho();

    inserirInicio(&lista, criarNo(r));

    mensagem("Remedio cadastrado com sucesso.");
}

void listarRemedios(Remedio *cabeca) {
    int linha = 4;

    limparTela();

    mvprintw(2, 4, "LISTAGEM DE REMEDIOS");

    if (cabeca == NULL) {
        mensagem("Nenhum remedio cadastrado.");
        return;
    }

    while (cabeca != NULL) {
        mvprintw(linha++, 4, "ID: %d", cabeca->id);
        mvprintw(linha++, 4, "Nome: %s", cabeca->nome);
        mvprintw(linha++, 4, "Laboratorio: %s", cabeca->laboratorio);
        mvprintw(linha++, 4, "Preco: %.2f", cabeca->preco);
        mvprintw(linha++, 4, "Quantidade: %d", cabeca->quantidade);
        mvprintw(linha++, 4, "-----------------------------");

        if (linha > LINES - 5) {
            pausar();
            limparTela();
            linha = 4;
        }

        cabeca = cabeca->prox;
    }

    pausar();
}

void editarRemedio() {
    int id;
    Remedio *r;

    limparTela();

    mvprintw(2, 4, "EDITAR REMEDIO");

    echo();

    mvprintw(4, 4, "Informe o ID: ");
    scanw("%d", &id);

    r = buscarPorId(lista, id);

    if (r == NULL) {
        noecho();
        mensagem("Remedio nao encontrado.");
        return;
    }

    mvprintw(6, 4, "Novo nome: ");
    getnstr(r->nome, 79);

    mvprintw(7, 4, "Novo laboratorio: ");
    getnstr(r->laboratorio, 79);

    mvprintw(8, 4, "Novo preco: ");
    scanw("%f", &r->preco);

    mvprintw(9, 4, "Nova quantidade: ");
    scanw("%d", &r->quantidade);

    noecho();

    mensagem("Remedio alterado com sucesso.");
}

void excluirRemedio() {
    int id;
    Remedio *atual = lista;
    Remedio *anterior = NULL;

    limparTela();

    mvprintw(2, 4, "EXCLUIR REMEDIO");

    echo();

    mvprintw(4, 4, "Informe o ID: ");
    scanw("%d", &id);

    noecho();

    while (atual != NULL && atual->id != id) {
        anterior = atual;
        atual = atual->prox;
    }

    if (atual == NULL) {
        mensagem("Remedio nao encontrado.");
        return;
    }

    if (anterior == NULL) {
        lista = atual->prox;
    } else {
        anterior->prox = atual->prox;
    }

    atual->prox = NULL;
    inserirInicio(&lixeira, atual);

    mensagem("Remedio enviado para a lixeira.");
}

void desfazerExclusao() {
    Remedio *restaurado;

    limparTela();

    if (lixeira == NULL) {
        mensagem("Lixeira vazia.");
        return;
    }

    restaurado = lixeira;
    lixeira = lixeira->prox;

    restaurado->prox = NULL;
    inserirInicio(&lista, restaurado);

    mensagem("Ultima exclusao desfeita.");
}

void salvarLista(const char *arquivo, Remedio *cabeca) {
    FILE *f = fopen(arquivo, "wb");

    if (f == NULL) {
        return;
    }

    while (cabeca != NULL) {
        fwrite(cabeca, sizeof(Remedio), 1, f);
        cabeca = cabeca->prox;
    }

    fclose(f);
}

void carregarLista(const char *arquivo, Remedio **cabeca) {
    FILE *f = fopen(arquivo, "rb");
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

void salvarTudo() {
    salvarLista(ARQ_REMEDIOS, lista);
    salvarLista("lixeira.dat", lixeira);
}

void carregarTudo() {
    carregarLista(ARQ_REMEDIOS, &lista);
    carregarLista("lixeira.dat", &lixeira);
}

void pesquisarPorNome() {
    char nome[80];
    Remedio *atual = lista;
    int linha = 6;
    int encontrou = 0;

    limparTela();

    mvprintw(2, 4, "PESQUISAR REMEDIO POR NOME");

    echo();
    mvprintw(4, 4, "Nome: ");
    getnstr(nome, 79);
    noecho();

    while (atual != NULL) {
        if (strstr(atual->nome, nome) != NULL) {
            mvprintw(linha++, 4, "ID: %d | Nome: %s | Preco: %.2f",
                     atual->id, atual->nome, atual->preco);
            encontrou = 1;
        }

        atual = atual->prox;
    }

    if (!encontrou) {
        mvprintw(6, 4, "Nenhum remedio encontrado.");
    }

    pausar();
}

int menuPrincipal() {
    int opcao;

    limparTela();

    mvprintw(2, 4, "SISTEMA DE FARMACIA");
    mvprintw(4, 4, "1 - Cadastrar remedio");
    mvprintw(5, 4, "2 - Listar remedios");
    mvprintw(6, 4, "3 - Editar remedio");
    mvprintw(7, 4, "4 - Excluir remedio");
    mvprintw(8, 4, "5 - Desfazer ultima exclusao");
    mvprintw(9, 4, "6 - Pesquisar por nome");
    mvprintw(10, 4, "7 - Listar lixeira");
    mvprintw(11, 4, "0 - Salvar e sair");

    mvprintw(13, 4, "Escolha uma opcao: ");

    echo();
    scanw("%d", &opcao);
    noecho();

    return opcao;
}

int main() {
    int opcao;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    criarUsuariosPadrao();

    if (!telaLogin()) {
        limparTela();
        mvprintw(4, 4, "Usuario ou senha invalidos.");
        pausar();
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
                listarRemedios(lista);
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
                listarRemedios(lixeira);
                break;
            case 0:
                salvarTudo();
                break;
            default:
                mensagem("Opcao invalida.");
        }

    } while (opcao != 0);

    endwin();

    printf("Dados salvos com sucesso.\n");

    return 0;
}
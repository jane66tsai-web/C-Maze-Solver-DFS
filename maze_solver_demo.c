#include <stdio.h>
#include <string.h>

#define MAX_ROWS 100
#define MAX_COLS 100
#define MAX_CELLS (MAX_ROWS * MAX_COLS)
#define MAX_NODES (MAX_CELLS * 4)

#define START 'S'
#define WALL '+'
#define TRIED '.'
#define PATH 'O'
#define DEAD_END '-'

typedef struct {
    int row;
    int col;
} Position;

typedef struct {
    char grid[MAX_ROWS][MAX_COLS + 1];
    int rows;
    int cols;
    int start_row;
    int start_col;
} Maze;

typedef struct {
    int row;
    int col;
    int parent_index;
} SearchNode;

typedef struct {
    int data[MAX_NODES];
    int top;
} Stack;

void init_stack(Stack *s) {
    s->top = -1;
}

int is_empty(Stack *s) {
    return s->top == -1;
}

int push(Stack *s, int value) {
    if (s->top >= MAX_NODES - 1) {
        return 0;
    }
    s->top++;
    s->data[s->top] = value;
    return 1;
}

int pop(Stack *s) {
    if (is_empty(s)) {
        return -1;
    }
    return s->data[s->top--];
}

void remove_newline(char line[]) {
    int len;
    len = (int)strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[len - 1] = '\0';
        len--;
    }
}

int read_maze(const char *filename, Maze *maze) {
    FILE *fp;
    char line[MAX_COLS + 5];
    int r;
    int c;
    int len;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Cannot open maze file: %s\n", filename);
        return 0;
    }

    maze->rows = 0;
    maze->cols = 0;
    maze->start_row = -1;
    maze->start_col = -1;

    while (fgets(line, sizeof(line), fp) != NULL && maze->rows < MAX_ROWS) {
        remove_newline(line);
        len = (int)strlen(line);
        if (len > maze->cols) {
            maze->cols = len;
        }

        for (c = 0; c < len && c < MAX_COLS; c++) {
            maze->grid[maze->rows][c] = line[c];
            if (line[c] == START) {
                maze->start_row = maze->rows;
                maze->start_col = c;
            }
        }
        maze->grid[maze->rows][c] = '\0';
        maze->rows++;
    }

    fclose(fp);

    for (r = 0; r < maze->rows; r++) {
        len = (int)strlen(maze->grid[r]);
        for (c = len; c < maze->cols; c++) {
            maze->grid[r][c] = WALL;
        }
        maze->grid[r][maze->cols] = '\0';
    }

    if (maze->start_row == -1 || maze->start_col == -1) {
        printf("No starting point S found in the maze.\n");
        return 0;
    }

    return 1;
}

void print_line(void) {
    printf("------------------------------------------------------------\n");
}

void print_legend(void) {
    printf("Legend:\n");
    printf("  + = wall\n");
    printf("  S = start\n");
    printf("  O = final path\n");
    printf("  . = visited cell\n");
    printf("  blank space = road\n");
}

void print_maze(Maze *maze) {
    int r;
    for (r = 0; r < maze->rows; r++) {
        printf("%s\n", maze->grid[r]);
    }
}

int is_inside(Maze *maze, int row, int col) {
    return row >= 0 && row < maze->rows && col >= 0 && col < maze->cols;
}

int is_exit(Maze *maze, int row, int col) {
    return row == 0 || row == maze->rows - 1 || col == 0 || col == maze->cols - 1;
}

int is_available(Maze *maze, int row, int col) {
    if (!is_inside(maze, row, col)) {
        return 0;
    }
    return maze->grid[row][col] != WALL &&
           maze->grid[row][col] != TRIED &&
           maze->grid[row][col] != PATH &&
           maze->grid[row][col] != DEAD_END;
}

int add_node(SearchNode nodes[], int *node_count, int row, int col, int parent_index) {
    if (*node_count >= MAX_NODES) {
        return -1;
    }
    nodes[*node_count].row = row;
    nodes[*node_count].col = col;
    nodes[*node_count].parent_index = parent_index;
    (*node_count)++;
    return (*node_count) - 1;
}

int build_path(SearchNode nodes[], int exit_index, Position path[], int *path_length) {
    Position reverse_path[MAX_CELLS];
    int count;
    int index;
    int i;

    count = 0;
    index = exit_index;

    while (index != -1 && count < MAX_CELLS) {
        reverse_path[count].row = nodes[index].row;
        reverse_path[count].col = nodes[index].col;
        count++;
        index = nodes[index].parent_index;
    }

    for (i = 0; i < count; i++) {
        path[i] = reverse_path[count - 1 - i];
    }

    *path_length = count;
    return 1;
}

int solve_maze_dfs(Maze *maze, Position path[], int *path_length, int *visited_count) {
    Stack stack;
    SearchNode nodes[MAX_NODES];
    int node_count;
    int start_index;
    int current_index;
    int row;
    int col;
    int directions[4][2];
    int i;
    int nr;
    int nc;
    int new_index;
    int pr;
    int pc;

    directions[0][0] = 0;  directions[0][1] = 1;   /* right */
    directions[1][0] = 0;  directions[1][1] = -1;  /* left */
    directions[2][0] = 1;  directions[2][1] = 0;   /* down */
    directions[3][0] = -1; directions[3][1] = 0;   /* up */

    init_stack(&stack);
    node_count = 0;
    *path_length = 0;
    *visited_count = 0;

    start_index = add_node(nodes, &node_count, maze->start_row, maze->start_col, -1);
    push(&stack, start_index);

    while (!is_empty(&stack)) {
        current_index = pop(&stack);
        row = nodes[current_index].row;
        col = nodes[current_index].col;

        if (!is_available(maze, row, col)) {
            continue;
        }

        (*visited_count)++;

        if (is_exit(maze, row, col) && maze->grid[row][col] != START) {
            build_path(nodes, current_index, path, path_length);

            for (i = 0; i < *path_length; i++) {
                pr = path[i].row;
                pc = path[i].col;
                maze->grid[pr][pc] = PATH;
            }

            maze->grid[maze->start_row][maze->start_col] = START;
            return 1;
        }

        maze->grid[row][col] = TRIED;

        /*
         * Stack is Last-In, First-Out.
         * To explore in the order up, down, left, right,
         * we push in reverse order: right, left, down, up.
         */
        for (i = 0; i < 4; i++) {
            nr = row + directions[i][0];
            nc = col + directions[i][1];
            if (is_available(maze, nr, nc)) {
                new_index = add_node(nodes, &node_count, nr, nc, current_index);
                if (new_index != -1) {
                    push(&stack, new_index);
                }
            }
        }
    }

    return 0;
}

void print_path(Position path[], int path_length) {
    int i;
    for (i = 0; i < path_length; i++) {
        printf("(%d, %d)", path[i].row, path[i].col);
        if (i != path_length - 1) {
            printf(" -> ");
        }
        if ((i + 1) % 5 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

int compare_correct_path(const char *filename, Position path[], int path_length) {
    FILE *fp;
    int row;
    int col;
    int index;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Cannot open correct path file: %s\n", filename);
        return 0;
    }

    index = 0;
    while (fscanf(fp, "%d %d", &row, &col) == 2) {
        if (index >= path_length) {
            fclose(fp);
            return 0;
        }
        if (path[index].row != row || path[index].col != col) {
            fclose(fp);
            return 0;
        }
        index++;
    }

    fclose(fp);
    return index == path_length;
}

void choose_file(char maze_file[], char correct_file[]) {
    int choice;

    printf("Maze Solver Demo\n");
    print_line();
    printf("Please choose a maze file:\n");
    printf("1. maze1.txt \n");
    printf("2. maze2.txt \n");
    printf("3. maze3.txt \n");
    printf("Your choice: ");

    if (scanf("%d", &choice) != 1) {
        choice = 1;
    }

    if (choice == 2) {
        strcpy(maze_file, "maze2.txt");
        strcpy(correct_file, "correct_path2.txt");
    } else if (choice == 3) {
        strcpy(maze_file, "maze3.txt");
        strcpy(correct_file, "");
    } else {
        strcpy(maze_file, "maze1.txt");
        strcpy(correct_file, "correct_path1.txt");
    }
}

void wait_before_exit(void) {
    int ch;
    printf("\nPress Enter to exit...");
    while ((ch = getchar()) != '\n' && ch != EOF) {
        ;
    }
    getchar();
}

int main(int argc, char *argv[]) {
    Maze maze;
    Position path[MAX_CELLS];
    int path_length;
    int visited_count;
    int found;
    char maze_file[100];
    char correct_file[100];

    path_length = 0;
    visited_count = 0;
    correct_file[0] = '\0';

    if (argc >= 2) {
        strcpy(maze_file, argv[1]);
        if (argc >= 3) {
            strcpy(correct_file, argv[2]);
        }
    } else {
        choose_file(maze_file, correct_file);
    }

    print_line();
    printf("Input file: %s\n", maze_file);
    if (correct_file[0] != '\0') {
        printf("Answer file: %s\n", correct_file);
    }
    print_line();

    if (!read_maze(maze_file, &maze)) {
        wait_before_exit();
        return 1;
    }

    print_legend();
    print_line();
    printf("Original Maze:\n");
    print_maze(&maze);

    found = solve_maze_dfs(&maze, path, &path_length, &visited_count);

    print_line();
    printf("Search Result:\n");
    if (found) {
        printf("Path found!\n");
        printf("Path length: %d\n", path_length);
        printf("Visited cells: %d\n", visited_count);
        printf("\nPath coordinates:\n");
        print_path(path, path_length);
        printf("\nSolved Maze:\n");
        print_maze(&maze);
    } else {
        printf("No path found.\n");
        printf("Visited cells: %d\n", visited_count);
        printf("\nMaze after search:\n");
        print_maze(&maze);
    }

    if (correct_file[0] != '\0') {
        print_line();
        printf("Checking with correct path file...\n");
        if (found && compare_correct_path(correct_file, path, path_length)) {
            printf("Result: The search path matches the correct path!\n");
        } else {
            printf("Result: The search path does not match the correct path.\n");
        }
    }

    print_line();
    wait_before_exit();
    return 0;
}

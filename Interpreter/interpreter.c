/*
    CSE360- Project.
    Single-Register Assembly Language Interpreter in C
	By Group-2
		Saif Khan	             2022-2-60-063
		Mohammad Ashiquzzaman Hadi   2021-3-60-143
		Abrar Hossain Zahin	     2022-2-60-040
		Rafid Rahman	             2022-2-60-040

    Run Command:
    gcc -o interpreter interpreter.c -lncurses
    ./interpreter
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>

#include <unistd.h>

#define MAX_LINES 100
#define MAX_LABEL_LEN 20
#define MAX_LINE_LEN 50
#define MAX_MEM 10

typedef struct
{
  char label[MAX_LABEL_LEN];
  char opcode[10];
  char operand_str[20];
  int operand;
  int has_operand;
  int is_label_operand;
} Instruction;

Instruction program[MAX_LINES];
int memory[MAX_MEM] = {0};
int line_count = 0;
int accumulator = 0;
int log_win_height;
int acc_history[MAX_LINES] = {0}; 
int paused = 0;

typedef struct
{
  char label[MAX_LABEL_LEN];
  int index;
} LabelMap;

LabelMap label_map[MAX_LINES];
int label_count = 0;

char completed_instr[MAX_LINES][MAX_LINE_LEN];
int completed_count = 0;

int find_label_index(const char *label)
{
  for (int i = 0; i < label_count; i++)
  {
    if (strcmp(label_map[i].label, label) == 0)
      return label_map[i].index;
  }
  return -1;
}

void add_label(const char *label, int index)
{
  strcpy(label_map[label_count].label, label);
  label_map[label_count].index = index;
  label_count++;
}

int is_number(const char *str)
{
  for (int i = 0; str[i]; i++)
  {
    if (!isdigit(str[i]))
      return 0;
  }
  return 1;
}

void parse_program()
{
  FILE *fp = fopen("program.asm", "r");
  if (!fp)
  {
    perror("Failed to open program.asm");
    exit(1);
  }

  char line[MAX_LINE_LEN];
  while (fgets(line, sizeof(line), fp) && line_count < MAX_LINES)
  {
    line[strcspn(line, "\r\n")] = 0;

    if (line[0] == '\0' || strspn(line, " \t") == strlen(line))
      continue;
    if (line[0] == ';')
      continue;

    Instruction instr = {"", "", "", 0, 0, 0};
    char label[MAX_LABEL_LEN] = "";
    char opcode[10] = "";
    char operand_str[20] = "";

    if (strchr(line, ':'))
    {
      sscanf(line, "%[^:]: %s %s", label, opcode, operand_str);
      add_label(label, line_count);
    }
    else
    {
      sscanf(line, "%s %s", opcode, operand_str);
    }

    if (label[0] != '\0')
      strcpy(instr.label, label);
    strcpy(instr.opcode, opcode);

    if (strcmp(opcode, "HALT") != 0 && strlen(operand_str) > 0)
    {
      instr.has_operand = 1;
      strcpy(instr.operand_str, operand_str);
      if (is_number(operand_str))
      {
        instr.operand = atoi(operand_str);
        instr.is_label_operand = 0;
      }
      else
      {
        instr.is_label_operand = 1;
      }
    }

    program[line_count++] = instr;

    if (strcmp(opcode, "HALT") == 0)
      break;
  }

  fclose(fp);
}
void draw_ui(WINDOW *header_win, WINDOW *mem_win, WINDOW *inst_win, WINDOW *acc_win, WINDOW *log_win, int pc)
{
    werase(mem_win);
    werase(inst_win);
    werase(acc_win);
    werase(log_win);

    mvwprintw(header_win, 0, 1, "Assembly Code Interpreter using Only Accumulator");
    wrefresh(header_win);

    int mem_win_height = getmaxy(mem_win);
    int max_mem_lines = mem_win_height - 2;
    if (max_mem_lines > MAX_MEM)
        max_mem_lines = MAX_MEM;

    wattron(mem_win, COLOR_PAIR(2)); 
    box(mem_win, 0, 0);
    wattroff(mem_win, COLOR_PAIR(2));

    wattron(mem_win, COLOR_PAIR(1)); 
    mvwprintw(mem_win, 0, 3, " Memory Blocks ");
    wattroff(mem_win, COLOR_PAIR(1));

    for (int i = 0; i < max_mem_lines; i++)
    {
        if (program[pc].has_operand && program[pc].operand == i)
        {
            wattron(mem_win, COLOR_PAIR(4)); 
            mvwprintw(mem_win, 1 + i, 1, ">%2d | %11d", i, memory[i]);
            wattroff(mem_win, COLOR_PAIR(4));
        }
        else
        {
            wattron(mem_win, COLOR_PAIR(1));
            mvwprintw(mem_win, 1 + i, 1, " %2d | %11d", i, memory[i]);
            wattroff(mem_win, COLOR_PAIR(1));
        }
    }

    wattron(inst_win, COLOR_PAIR(3)); 
    box(inst_win, 0, 0);
    wattroff(inst_win, COLOR_PAIR(3));

    wattron(inst_win, COLOR_PAIR(3));
    mvwprintw(inst_win, 0, 8, " Line %d ", pc + 1);
    mvwprintw(inst_win, 1, 2, "Opcode        Operand");

    char operand_print[20] = "";
    if (program[pc].has_operand)
    {
        strncpy(operand_print, program[pc].operand_str, sizeof(operand_print) - 1);
        operand_print[sizeof(operand_print) - 1] = '\0';
    }

    mvwprintw(inst_win, 2, 2, "%-12s %s", program[pc].opcode, operand_print);
    wattroff(inst_win, COLOR_PAIR(3));

    wattron(acc_win, COLOR_PAIR(5)); 
    box(acc_win, 0, 0);
    mvwprintw(acc_win, 0, 6, " Accumulator ");
    mvwprintw(acc_win, 1, 2, "%10d", accumulator);
    wattroff(acc_win, COLOR_PAIR(5));

    wattron(log_win, COLOR_PAIR(1)); 
    box(log_win, 0, 0);
    mvwprintw(log_win, 0, 12, " Completed Instructions (%d) ", completed_count / 2);
    wattroff(log_win, COLOR_PAIR(1));

    int log_win_height = getmaxy(log_win);
    int max_log_lines = log_win_height - 2;

    for (int i = 0; i < completed_count && i < max_log_lines; i++)
    {
        int color = 2; 
        if (strstr(completed_instr[i], "ADD"))
            color = 3;
        else if (strstr(completed_instr[i], "SUB"))
            color = 4;
        else if (strstr(completed_instr[i], "LOAD"))
            color = 5;
        else if (strstr(completed_instr[i], "STORE"))
            color = 1;
        else if (strstr(completed_instr[i], "JUMPP") || strstr(completed_instr[i], "JUMP "))
            color = 6;
        else if (strstr(completed_instr[i], "JUMPN"))
            color = 7;
        else if (strstr(completed_instr[i], "JUMPZ"))
            color = 8;
        else if (strstr(completed_instr[i], "HALT"))
            color = 9;

        wattron(log_win, COLOR_PAIR(color));
        mvwprintw(log_win, 1 + i, 7, "%20s | ACC: %d", completed_instr[i], acc_history[i]);
        wattroff(log_win, COLOR_PAIR(color));
    }

    wrefresh(mem_win);
    wrefresh(inst_win);
    wrefresh(acc_win);
    wrefresh(log_win);
}


void execute_program(WINDOW *header_win, WINDOW *mem_win, WINDOW *inst_win, WINDOW *acc_win, WINDOW *log_win)
{
  int running = 1;
  int paused = 0;

  nodelay(stdscr, TRUE);

  while (running)
  {
    int pc = 0;
    completed_count = 0;

    while (pc < line_count)
    {
      Instruction instr = program[pc];
      int target;

      if (log_win)
        delwin(log_win);
      log_win_height = completed_count + 2;
      if (log_win_height > LINES - 15)
        log_win_height = LINES - 15;
      log_win = newwin(log_win_height, 50, 14, 1);

      draw_ui(header_win,mem_win, inst_win, acc_win, log_win, pc);
      usleep(1000000);

      int ch = getch();
      if (ch == 'p' || ch == 'P')
      {
        paused = !paused;
        if (paused)
        {
          int y_prompt = getbegy(log_win) + getmaxy(log_win) + 1;
          mvprintw(y_prompt, 1, "Paused. Press 'p' to resume.");
          refresh();
        }
      }

      while (paused)
      {
        int pause_ch = getch();
        if (pause_ch == 'p' || pause_ch == 'P')
        {
          paused = 0;
          clear();
          refresh();
          break;
        }
        usleep(100000);
      }

char log_line[60];
snprintf(log_line, sizeof(log_line), " %-4s %d-> %s %s","Line", pc + 1, instr.opcode,
         instr.has_operand ? instr.operand_str : "");

      strcpy(completed_instr[completed_count], log_line);
      acc_history[completed_count] = accumulator;
      completed_count++;

      if (strcmp(instr.opcode, "LOAD") == 0 && instr.has_operand)
      {
        if (instr.operand >= 0 && instr.operand < MAX_MEM)
          accumulator = memory[instr.operand];
      }
      else if (strcmp(instr.opcode, "STORE") == 0 && instr.has_operand)
      {
        if (instr.operand >= 0 && instr.operand < MAX_MEM)
          memory[instr.operand] = accumulator;
      }
      else if (strcmp(instr.opcode, "ADD") == 0 && instr.has_operand)
      {
        if (instr.operand >= 0 && instr.operand < MAX_MEM)
          accumulator += memory[instr.operand];
      }
      else if (strcmp(instr.opcode, "SUB") == 0 && instr.has_operand)
      {
        if (instr.operand >= 0 && instr.operand < MAX_MEM)
          accumulator -= memory[instr.operand];
      }
      else if (strcmp(instr.opcode, "JUMP") == 0 && instr.has_operand)
      {
        target = instr.is_label_operand ? find_label_index(instr.operand_str) : instr.operand;
        if (target >= 0 && target < line_count)
        {
          completed_count++;
          acc_history[completed_count - 1] = accumulator;
          pc = target;
          continue;
        }
      }
      else if (strcmp(instr.opcode, "JUMPZ") == 0 && instr.has_operand)
      {
        if (accumulator == 0)
        {
          target = instr.is_label_operand ? find_label_index(instr.operand_str) : instr.operand;
          if (target >= 0 && target < line_count)
          {
            completed_count++;
            acc_history[completed_count - 1] = accumulator;
            pc = target;
            continue;
          }
        }
      }
      else if (strcmp(instr.opcode, "JUMPN") == 0 && instr.has_operand)
      {
        if (accumulator < 0)
        {
          target = instr.is_label_operand ? find_label_index(instr.operand_str) : instr.operand;
          if (target >= 0 && target < line_count)
          {
            completed_count++;
            acc_history[completed_count - 1] = accumulator;
            pc = target;
            continue;
          }
        }
      }
      else if (strcmp(instr.opcode, "JUMPP") == 0 && instr.has_operand)
      {
        if (accumulator > 0)
        {
          target = instr.is_label_operand ? find_label_index(instr.operand_str) : instr.operand;
          if (target >= 0 && target < line_count)
          {
            completed_count++;
            acc_history[completed_count - 1] = accumulator;
            pc = target;
            continue;
          }
        }
      }
      else if (strcmp(instr.opcode, "HALT") == 0)
      {
        break;
      }else{
       printf("\n\nError: Invalid opcode \"%s\" at line %d.\n", instr.opcode, pc);
    	break;
      }

      acc_history[completed_count] = accumulator;
      completed_count++;

      pc++;
      clear();
      refresh();
    }
    int y_prompt = getbegy(log_win) + getmaxy(log_win) + 1;
    mvprintw(y_prompt, 1, "Program halted. Press 'r' to reload, 'ESC' to exit.");
    refresh();

    int ch;
    while ((ch = getch()))
    {
      if (ch == 27)
      { 
        running = 0;
        break;
      }
      else if (ch == 'r' || ch == 'R')
      {
        parse_program();
        break;
      }
    }
  }
}

int main()
{

  memory[0] = 10;
  memory[1] = 20;
  memory[2] = 30;
  memory[3] = 40;
  memory[4] = 50;
  memory[5] = 60;
  memory[6] = 70;
  memory[7] = 80;
  memory[8] = 90;
  memory[9] = 100;

  initscr(); 
  noecho();
  cbreak();
  curs_set(0);

  start_color(); 

  init_pair(1, COLOR_YELLOW, COLOR_BLACK); 
  init_pair(2, COLOR_CYAN, COLOR_BLACK);   
  init_pair(3, COLOR_GREEN, COLOR_BLACK);  
  init_pair(4, COLOR_RED, COLOR_BLACK);    
  init_pair(5, COLOR_MAGENTA, COLOR_BLACK);

  init_pair(6, COLOR_BLUE, COLOR_BLACK); 
  init_pair(7, COLOR_WHITE, COLOR_BLACK); 
  init_pair(8, COLOR_RED, COLOR_BLACK); 
  init_pair(9, COLOR_GREEN, COLOR_WHITE); 

  WINDOW *header_win = newwin(1, 50, 0, 0);
  WINDOW *mem_win = newwin(12, 20, 1, 1);
  WINDOW *inst_win = newwin(4, 25, 1, 25);
  WINDOW *acc_win = newwin(3, 25, 10, 25);
  WINDOW *log_win = newwin(12, 50, 14, 1);

  parse_program();
  execute_program(header_win,mem_win, inst_win, acc_win, log_win);

  delwin(mem_win);
  delwin(inst_win);
  delwin(acc_win);
  delwin(log_win);
  endwin();

  return 0;
}

/*
gcc interpreter.c -o interpreter -lncurses
./interpreter
*/
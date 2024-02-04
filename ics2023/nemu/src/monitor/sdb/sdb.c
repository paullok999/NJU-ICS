/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/vaddr.h>


static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  //set nemu_state to NEMU_END before quit
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_si(char *args){
  int num = 0;
  //default 1
  if(args == NULL){
    num = 1;
  }else{
    //parse args to int type
    num = atoi(args);
  }
  if(num > 0){
    cpu_exec(num);
  }else{
    printf("invalid argument:%s\n",args);
  }
  return 0;
}

static int cmd_info(char *args){
  if(args == NULL || strlen(args) > 1){
    printf("invalid argument\n");
    return 0;
  }
  switch(args[0]){
    case 'r':
	//print registers status
	isa_reg_display();	
	break;
    case 'w':
	//TODO:print watch points status

	break;
    default:
	printf("the new function waited to be implement...\n");
	break;
  }
  return 0;
}

static int cmd_x(char *args){
  if(!args){
    printf("You must provide argument\n");
    return 1;
  }
  //recognize two arguments
  char *count = strtok(args," ");
  char *expr = strtok(NULL," ");
  if(!count || !expr){
    printf("Invalid arguments\n");
    return 1;
  }
  vaddr_t va;
  //TODO:you should call function to parse
  int res = sscanf(expr,"%x",&va);
  if(res <= 0){
    printf("Address convert failed\n");
    return 1;
  }
  int len = atoi(count);
  word_t val;
  for(int i = 0;i < len;i++){
      if(i != 0 && i % 4 == 0)printf("\n");
      if(i % 4 == 0)printf("0x%08x: ",va);
      //access memory
      val = vaddr_read(va,1);
      printf("0x%02x    ",val); 
      va += 1;
  }
  printf("\n");
  return 0;
}

static int cmd_p(char *args){
  if(!args){
    printf("You must provide argument\n");
    return 1;
  }
  bool is_success = false;
  word_t res_of_expr = expr(args,&is_success);
  if(is_success){
    printf("%d\n",res_of_expr);
  }else{
    printf("evaluation failed\n");
    return 1;
  }
  
  return 0;
}

static int cmd_w(char *args){
  return 0;
}

static int cmd_d(char *args){
  return 0;
}



static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  /* TODO: Add more commands */
  { "si","Step into,format:si [N],N default 1",cmd_si},
  { "info","Print program status,format:info SUBCMD",cmd_info},
  { "x","Scan memory,format:x N EXPR",cmd_x},
  { "p","Expression evaluation,format:p EXPR",cmd_p},
  { "w","Set watch point,format:w EXPR",cmd_w},
  { "d","Delete watch point N,format:d N",cmd_d}
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}



void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 80 // 명령어 최대 길이

// 앞뒤 공백 제거
void trim(char *str)
{
  char *start = str;                 // 앞 공백 위치
  char *end = str + strlen(str) - 1; // 뒤 공백 위치
  while (isspace(*start))
  { // 앞 공백 제거
    start++;
  }
  while (isspace(*end))
  { // 뒤 공백 제거
    end--;
  }
  *(end + 1) = '\0';                    // 뒤 공백 삭제
  memmove(str, start, end - start + 2); // 앞뒤 공백 삭제
}
// 명령어를 분리하기
void tokenize(char *line, char *args_tok[])
{
  int i = 0;
  char *token = strtok(line, " \t\r\n"); // 공백을 기준으로 분리
  while (token != NULL)
  { // 명령어 분리
    args_tok[i++] = token;
    token = strtok(NULL, " \t\r\n"); // 다음 명령어 분리
  }
  args_tok[i] = NULL;
}
void myPipe(char *line)
{                                                  // 파이프
  int fd[2];                                       // 파이프 입력 출력 파일 디스크립터
  int pid;                                         // 프로세스 아이디
  char *command1, *command2;                       // 파이프 입력 명령어
  char *args_tok1[MAX_LINE], *args_tok2[MAX_LINE]; // 명령어 분리
  // 파이프 입력 명령어 저장
  command1 = strtok(line, "|"); // command1에 파이프 입력 명령어 저장
  command2 = strtok(NULL, "|"); // command2에 파이프 출력 명령어 저장

  // 공백 제거
  trim(command1);
  trim(command2);

  // 파이프 입력 명령어 분리
  tokenize(command1, args_tok1);
  tokenize(command2, args_tok2);
  pipe(fd); // 파이프 생성
  pid = fork();

  if (pid == 0)
  {                 // 자식 프로세스
    close(fd[0]);   // 자식 프로세스의 입력 파이프를 닫음
    dup2(fd[1], 1); // 파이프 출력 디스크립터로 설정
    close(fd[1]);   // 자식 프로세스의 출력 파이프를 닫음
    if (execvp(args_tok1[0], args_tok1) < 0)
    { // 자식 프로세스 실행
      printf("%s: Command not found\n", args_tok1[0]);
      exit(0);
    }
  }
  else
  {                 // 부모 프로세스
    close(fd[1]);   // 파이프 입력 디스크립터 닫기
    dup2(fd[0], 0); // 파이프 입력 디스크립터로 설정
    close(fd[0]);   // 파이프 입력 디스크립터 닫기
    if (execvp(args_tok2[0], args_tok2) < 0)
    { // 부모 프로세스 실행
      printf("%s: Command not found\n", args_tok2[0]);
      exit(0);
    }
  }

  exit(0);
}

void myRedirection1(char *line)
{ // 입력 리다이렉션
  int fd;
  int pid;
  char *command;
  char *file_name;
  char *args[MAX_LINE];

  command = strtok(line, ">");
  file_name = strtok(NULL, ">");

  trim(command);
  trim(file_name);

  tokenize(command, args);

  pid = fork();

  if (pid == 0)
  {
    fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644); // 파일 열기
    dup2(fd, STDOUT_FILENO);
    close(fd);
    if (execvp(args[0], args) < 0)
    {
      printf("%s: Command not found\n", args[0]);
      exit(0);
    }
  }
  else
  {
    wait(NULL);
  }
}

void myRedirection2(char *line)
{ // 출력 리다이렉션
  int fd;
  int pid;
  char *command;
  char *file_name;
  char *args[MAX_LINE];

  command = strtok(line, "<");
  file_name = strtok(NULL, "<");

  trim(command);
  trim(file_name);

  tokenize(command, args);

  pid = fork();

  if (pid == 0)
  {
    fd = open(file_name, O_RDONLY); // 파일 열기
    dup2(fd, STDIN_FILENO);
    close(fd);
    if (execvp(args[0], args) < 0)
    {
      printf("%s: Command not found\n", args[0]);
      exit(0);
    }
  }
  else
  {
    wait(NULL);
  }
}

void myExecute(char *line)
{ // 명령어 실행 및 백그라운드 실행
  int pid;
  char *args[MAX_LINE];
  int isbg;
  int status;

  if (strchr(line, '&') != NULL)
  { // 백그라운드 실행인지 확인
    isbg = 1;
    strtok(line, "&");
    trim(line);
    tokenize(line, args);
  }
  else
  {
    isbg = 0;
    tokenize(line, args);
  }

  pid = fork();

  if (pid == 0)
  {
    if (execvp(args[0], args) < 0)
    {
      printf("%s: Command not found\n", args[0]);
      exit(0);
    }
  }
  else
  {
    if (isbg == 0)
    {
      wait(&status);
    }
  }
}

int main()
{
  char *line;                                            // 명령어를 입력받을 변수
  char *args_tok[MAX_LINE];                              // 명령어를 분리할 변수
  int pid;                                               // 가장 처음 실행되는 명령어의 pid
  char *command1, *command2;                             // command1은 첫번째 명령어, command2는 두번째 명령어
  char *command1_tok[MAX_LINE], *command2_tok[MAX_LINE]; // command1의 토큰, command2의 토큰
  char *file_name;                                       // 파일 이름
  int isbg = 0;                                          // 백그라운드 여부
  while (1)
  {                                                 // 쉘 시작
    line = (char *)malloc(sizeof(char) * MAX_LINE); // 명령어 입력 받을 변수 초기화
    fgets(line, MAX_LINE, stdin);                   // 명령어 입력
    line[strlen(line) - 1] = '\0';                  // 마지막 개행문자 삭제

    if (strcmp(line, "exit") == 0)
    { // exit를 입력하면 종료
      break;
    }

    if (strcmp(line, "logout") == 0)
    { // logout를 입력하면 종료
      break;
    }

    // >가 있는지 확인(리다이렉션)
    if (strchr(line, '>') != NULL)
    {
      myRedirection1(line); // 리다이렉션
      continue;
    }
    // <가 있는지 확인(리다이렉션)
    if (strchr(line, '<') != NULL)
    {
      myRedirection2(line); // 리다이렉션
      continue;
    }
    // |가 있는지 확인(파이프 실행)
    if (strchr(line, '|') != NULL)
    {
      pid = fork();
      if (pid == 0)
      {
        // 자식 프로세스
        myPipe(line);
      }
      else
      {
        // 부모 프로세스
        wait(NULL);
      }
      continue;
    }

    myExecute(line);
  }
}
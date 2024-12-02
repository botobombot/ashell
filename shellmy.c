#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>
#include <signal.h>

#define SSYMBS1 "()|&;><\0"
#define SSYMBS2 "|&>\0"

#define REALOC_SETUP 8
#define REALOC_STEP 4

#define RED_TEXT "\033[1;31m"
#define GREEN_TEXT "\033[1;32m"
#define YELLOW_TEXT "\033[1;33m"
#define BLUE_TEXT "\033[1;34m"
#define MAGENTA_TEXT "\033[1;35m"
#define CYAN_TEXT "\033[1;36m"
#define WHITE_TEXT "\033[1;37m"
#define RESET_COLOR "\033[0m"


char** read_user_inp() // функция разбивает ввод пользователя на отдельные команды
{
                                                //то что ввел пользователь пишем в raw_user_inp 
    char *raw_user_inp = (char*)calloc(sizeof(char), REALOC_SETUP);         //создаем массив из указателей на строки
    if(raw_user_inp == NULL)
    {
        fprintf(stderr, RED_TEXT "ERROR: calloc error\n" RESET_COLOR); // типичные сообщения об ошибке
        return NULL;
    }
    int remained_space_count = REALOC_SETUP;
    int k = 0;                  
    char c;                                            //посимвольно читаем строку
    while ((c = getchar()) != '\n')
    {
        raw_user_inp[k] = c;
        k++;
        remained_space_count--;
        if (remained_space_count == 0)
        {
            raw_user_inp = (char*)realloc(raw_user_inp, k + REALOC_SETUP);
            if(raw_user_inp == NULL)
            {
                fprintf(stderr, RED_TEXT "ERROR: realloc error\n" RESET_COLOR);
                return NULL;
            }
            remained_space_count += REALOC_STEP;
        }
    }
    if(k == 0)
    {
        // на 0 и суда NULL        
        free(raw_user_inp);
        return NULL;
    }
    raw_user_inp[k] = 0; 
    char* correct_user_inp = (char*)calloc(strlen(raw_user_inp)*2 + 1, sizeof(char)); // для дальнейшего разбиения переделываем строку
    if(correct_user_inp == NULL)
    {
        fprintf(stderr, RED_TEXT "ERROR: calloc error\n" RESET_COLOR);
        return NULL;
    }
    k = 0;
    for(int i = 0; i < strlen(raw_user_inp); i++)
    {
        if((strchr(SSYMBS2, raw_user_inp[i]) != NULL) && (raw_user_inp[i] == raw_user_inp[i+1]))
        {
            correct_user_inp[k] = ' ';
            correct_user_inp[k+1] = raw_user_inp[i];
            correct_user_inp[k+2] = raw_user_inp[i+1]; // просто добавляем пробелы
            correct_user_inp[k+3] = ' ';
            i++;
            k += 4;
        }
        else if(strchr(SSYMBS1, raw_user_inp[i]) != NULL)
        {
            correct_user_inp[k] = ' ';
            correct_user_inp[k+1] = raw_user_inp[i];
            correct_user_inp[k+2] = ' ';
            k += 3;
        }
        else
        {
            correct_user_inp[k] = raw_user_inp[i];
            k++;
        }
    }
    correct_user_inp[k] = 0;
    free(raw_user_inp);
    // создаём возвращаемый массив
    char** user_str = (char**)calloc(8, sizeof(char*));
    if(user_str == NULL)
    {
        fprintf(stderr, RED_TEXT "ERROR: calloc error\n" RESET_COLOR);
        free(correct_user_inp);
        return NULL;
    }
    remained_space_count = REALOC_SETUP;
    k = 0;
    raw_user_inp = strtok(correct_user_inp, " \0"); //разбиваем строку на части
    do
    {
        user_str[k] = (char*) calloc(strlen(raw_user_inp)+1, sizeof(char));
        strcpy(user_str[k], raw_user_inp); 
        k++;
        remained_space_count--; //всё та же схема
        if (remained_space_count == 0)
        {
            user_str = (char**)realloc(user_str, (k + REALOC_SETUP) * sizeof(char*));
            if(user_str == NULL)
            {
                fprintf(stderr, RED_TEXT "ERROR: realloc error\n" RESET_COLOR);
                return NULL;
            }
            remained_space_count += REALOC_STEP;
        }
        raw_user_inp = strtok(NULL, " \0"); //делим строку дальше
    }while(raw_user_inp != NULL);
    user_str[k] = raw_user_inp; // NULL
    free(correct_user_inp);
    free(raw_user_inp);
    return user_str;

}

void deleteL(char** delobj)
{
    for(int i = 0; delobj[i] != NULL; i++)
    {
        free(delobj[i]);
    }
    free(delobj);
}

int change_Dir(char* dir) // cd :)
{
    int temp;
    if ((temp = chdir(dir)) != 0)
    {
        fprintf(stderr, RED_TEXT "ERROR: no such directory %s\n" RESET_COLOR, dir);
    }
    return temp;
}

    //перенаправление ввода вывода (> <, >>)
int Redirection_Handler(char** redirection_list) // работаем с перенаправленным файлом
{  
    int file_d;
    int i = 0;
    while(redirection_list[i] != NULL)
    {
        if(strcmp(redirection_list[i], ">") == 0)
        {
            fprintf(stderr, YELLOW_TEXT "check\n" RESET_COLOR);
            file_d = open(redirection_list[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if(file_d == -1)
            {
                fprintf(stderr, RED_TEXT "ERROR: cannot open/create %s\n" RESET_COLOR, redirection_list[i+1]);
                return 1;
            }
            dup2(file_d, 1);
            close(file_d);
        }
        else if(strcmp(redirection_list[i], ">>") == 0)
        {
            file_d = open(redirection_list[i+1], O_WRONLY | O_APPEND | O_CREAT, 0666);
            if(file_d == -1)
            {
                fprintf(stderr, RED_TEXT "ERROR: cannot open/create %s\n" RESET_COLOR, redirection_list[i+1]);
                return 1;
            }
            
            lseek(file_d, SEEK_END, 0);
            dup2(file_d, 1);
            lseek(1, SEEK_END, 0);
            close(file_d);
        }
        else if(strcmp(redirection_list[i], "<") == 0)
        {
            file_d = open(redirection_list[i+1], O_RDONLY);
            if(file_d == -1)
            {
                fprintf(stderr, RED_TEXT "ERROR: cannot open/create %s\n" RESET_COLOR, redirection_list[i+1]);
                return 1;
            }
            dup2(file_d, 0);
            close(file_d);
        }
        i += 2;
    }
    return 0;
}

int interpretate_commands(char** list);

   //конвейер исполняемых команд
int command(char** list, char fl)
{
    if(strcmp(list[0], "cd") == 0)
    {
        if((list[1] == NULL) || (list[2] != NULL))
        {
            fprintf(stderr, RED_TEXT "ERROR: wrong amount of arguments in cd\n" RESET_COLOR);
            return 1;
        }
        else
        {
            return change_Dir(list[1]);
        }
    }
    if(strcmp(list[0], "(") == 0)
    {
        int k;
        for(k = 0; list[k] != NULL; k++);
        list[k-1] = NULL;
        return interpretate_commands(list+1);
    }
    char** redirection_list = (char**) calloc(5, sizeof(char*)); // имеет вид {>, file, < file, NULL}
    if(redirection_list == NULL){
        fprintf(stderr, RED_TEXT "ERROR: calloc error occurred\n" RESET_COLOR);
        return 1;
    }
    int i = 0;
    int j = 0;
    int size_of_command_list = 0;
    while(list[size_of_command_list++] != NULL); // длина списка
    char** pipeline_list = (char**) calloc(size_of_command_list, sizeof(char*));  //конвейер
    if(pipeline_list == NULL)
    {
        fprintf(stderr, RED_TEXT "ERROR: calloc error occurred\n" RESET_COLOR);
        deleteL(redirection_list);
        return 1;
    }
    if((strcmp("<", list[0]) == 0) || (strcmp(">", list[0]) == 0) || (strcmp(">>", list[0]) == 0))
    {
        redirection_list[j] = list[0];
        redirection_list[j+1] = list[1];
        j += 2;
        i += 2;
        if((strcmp("<", list[2]) == 0) || (strcmp(">", list[2]) == 0) || (strcmp(">>", list[0]) == 0))
        {
            redirection_list[j++] = list[2];
            redirection_list[j++] = list[3];
            i+= 2;
        }
    }
    else if((size_of_command_list > 2) && ((strcmp("<", list[size_of_command_list-3]) == 0) || (strcmp(">", list[size_of_command_list-3]) == 0) || (strcmp(">>", list[size_of_command_list-3]) == 0))) // если в конце списка
    {
        redirection_list[j] = list[size_of_command_list-3];
        redirection_list[j+1] = list[size_of_command_list-2];
        j += 2;
        size_of_command_list -= 2;
        if((size_of_command_list > 2) && ((strcmp("<", list[size_of_command_list-3]) == 0) || (strcmp(">", list[size_of_command_list-3]) == 0) || (strcmp(">>", list[size_of_command_list-3]) == 0)))
        {
            redirection_list[j] = list[size_of_command_list-3];
            redirection_list[j+1] = list[size_of_command_list-2];
            j += 2;
            size_of_command_list -= 2;
        }
    }
    redirection_list[j] = NULL; // привели к виду {>, file, < file, NULL}
    j = 0;
    int pipeline_size = 1;
    for(i = i; i<size_of_command_list-1; i++) // сколько будет команд на конвеере???
    {
        if(strcmp(list[i], "|") == 0)
            pipeline_size++;
        pipeline_list[j] = list[i];
        j++;
    }
    pipeline_list[j] = NULL;
    char** temp_ppline = pipeline_list;
    pid_t pid_array[pipeline_size];
    int fd[2]; //для обмена через pipe
    for(i = 0; i < pipeline_size; i++)
    {
        pipe(fd);
        int prevfd;
        char** cur_command = (char**) calloc((size_of_command_list), sizeof(char*));
        if(cur_command == NULL)
        {
            fprintf(stderr, RED_TEXT "ERROR: calloc error occurred\n" RESET_COLOR);
            deleteL(redirection_list);
            free(pipeline_list);
            return 1;
        }
        int k = 0;
        while((temp_ppline[k] != NULL) && (strcmp(temp_ppline[k], "|") != 0)) // дробим на выр. вида: {{com1..}, |, {com2..}..}
        {
            cur_command[k] = temp_ppline[k];
            k++;
        }
        cur_command[k] = NULL;
        temp_ppline += k+1; // господи, спасибо что мы не в ассемблере и можем делать такие фокусы
        //{com1..} на исполнение
        if((pid_array[i] = fork()) == 0) // sonппппеапппппппппппппппппппппппппппппппппп
        {
            if(fl == 0)
            {
                signal(SIGINT, SIG_DFL); // кому как, а лично ему дефолтно на сигинт
            }
            if(Redirection_Handler(redirection_list) == 1)
            {
                fprintf(stderr, RED_TEXT "ERROR: redirection error\n" RESET_COLOR);
                exit(1);
            }
            if(i != 0)
            {
                dup2(prevfd, 0);
            }
            if(i != pipeline_size-1) // вся эта муть с настройкой передавания инфы через pipe
            {
                dup2(fd[1], 1);
            }
            close(fd[0]);
            close(fd[1]);
            execvp(cur_command[0], cur_command); // УУУУУУУРААААА EXEC НАКОНЕЦ-ТО ААААААААААААААААААААААААААА
            fprintf(stderr, RED_TEXT "ERROR: cannot execute %s\n" RESET_COLOR, cur_command[0]);
            exit(1);
        }
        if(pid_array[i] < 0)
        {
            fprintf(stderr, RED_TEXT "ERROR: fork() ret %d\n" RESET_COLOR, pid_array[i]);
            abort();
        }
        free(cur_command);ппппеапппппппппппппппппппппппппппппппппп
        prevfd = fd[0];
        close(fd[1]);
    }
    // вот тут конвеер своё отработал
    free(pipeline_list);
    free(redirection_list);
    for(i = 0; i < pipeline_size-1; i++)
    {
        waitpid(pid_array[i], NULL, 0); // ждём завершения каждой подзадачи
    }
    int pipeline_status;
    waitpid(pid_array[pipeline_size-1], &pipeline_status, 0);
    if(WIFEXITED(pipeline_status) == 0)
    {                
        if(WSTOPSIG(pipeline_status)) //возвращает номер сигнала, из-за которого сын был остановлен
            return WSTOPSIG(pipeline_status);
        fprintf(stderr, RED_TEXT "ERROR: pipeline executing error\n" RESET_COLOR);
        //perror("\n");
        return 1;
    }
    else
    {
        return WEXITSTATUS(pipeline_status);
    }

}

//отправляем текущую команду на проверку условий + выполнение
int check_condititons(char** list, char fl)
{
    int ex_status = 0;
    int condition = 0;
    char** curr_list2;
    int k = 0;
    int i, ok;
    while(1)
    {
        curr_list2 = (char**) calloc(256, sizeof(char*));
        if(curr_list2 == NULL)
        {
            fprintf(stderr, RED_TEXT "ERROR: calloc error\n" RESET_COLOR);
            return 1;
        }
        i = 0;
        ok = 0;
        if(strcmp(list[k], "(") == 0)
        {
            ok++;
            curr_list2[i] = list[k];
            i++;
            k++;
            while(1)
            {
                curr_list2[i] = list[k];
                i++;
                k++;
                if(strcmp(list[k-1], "(") == 0)
                {
                    ok++;
                }
                else if(strcmp(list[k-1], ")") == 0)
                {
                    ok--;
                }
                if(ok == 0)
                {
                    break;
                }
            }
        }
        else // все действия аналогичны таким же в interpretate_commands(), времени и сил делать красиво не было
        {
            while((list[k] != NULL) && (strcmp(list[k], "&&") != 0) && (strcmp(list[k], "||") != 0))
            {
                curr_list2[i++] = list[k++];
            }
        }
        curr_list2[i] = NULL;
        if(ex_status == condition)              //если все ok, то выполняем команду
            ex_status = command(curr_list2, fl);               
        if(list[k] == NULL)
            break;
        condition = strcmp(list[k], "&&") != 0;
        k++;
        free(curr_list2);
    }
    free(curr_list2);
    return ex_status;
}

    //фоновый режим 
int background_task_init(char** list) // решил делать по старинке, написав попроще
{
    pid_t pid;
    if((pid = fork()) == 0) // пустоватый son
    {
        if(fork() == 0) // ну ваще аж pra-son
        {
            signal(SIGINT, SIG_IGN);
            int dev_null = open("/dev/null", O_RDWR);   // перенаправление,  всё как в инструкции задания
            dup2(dev_null, 0);
            dup2(dev_null, 1);
            close(dev_null);
            check_condititons(list, 1);    //выполнение самого фонового процесса
            sleep(20); // втыкаем, ждём завершения на всякий случай
            exit(0);
        }
        if(pid < 0)
        {
            fprintf(stderr, RED_TEXT "ERROR: fork() ret %d\n" RESET_COLOR, pid);
            abort();
        }
        exit(0);
    }
    if(pid < 0)
    {
        fprintf(stderr, RED_TEXT "ERROR: fork() ret %d\n" RESET_COLOR, pid);
        abort();
    }
    int st;
    waitpid(pid, &st, 0);
    if(WIFEXITED(st) == 0)
        return 1;
    else
        return WEXITSTATUS(st);
}


int interpretate_commands(char** list) // выполнение команд (обработанного вывода пользователя)
{
    pid_t pid;
    if((pid = fork()) == 0) // son
    {
        int ex_status = 0;
        int bracket_balance; // баланс скобок
        int k = 0;
        int i, j; // i для list, j для curr_list
        while(1)
        {
            char** curr_list = (char**) calloc(256, sizeof(char*));
            i = k;
            j = 0;
            bracket_balance = 0;
            if(strcmp(list[i], "(") == 0) // (*something*) выделяем отдельно
            {
                bracket_balance++;
                curr_list[j] = list[i];
                j++;
                i++;
                while(1)
                {
                    curr_list[j] = list[i];
                    j++;
                    i++;
                    if(strcmp(list[i-1], "(") == 0)
                    {
                        bracket_balance++;
                    }
                    else if(strcmp(list[i-1], ")") == 0)
                    {
                        bracket_balance--;
                    }
                    if(bracket_balance == 0)
                    {
                        break;
                    }
                }
            }
            else
            {
                while(((list[i] != NULL) && (strcmp(list[i], "&") != 0) && (strcmp(list[i], ";") != 0)) || (bracket_balance != 0)) // выделяем отдельные команды или скобки
                {
                    if(strcmp(list[i], "(") == 0)
                        bracket_balance++;
                    if(strcmp(list[i], ")") == 0)
                        bracket_balance--;
                    curr_list[j] = list[i];
                    j++;
                    i++;
                }
            }
            curr_list[j] = NULL; // маркер конца списка
            if((list[i] != NULL) && (strcmp(list[i], "&") == 0)) //переход в фоновый режим
            {
                ex_status |= background_task_init(curr_list);
            }
            else
            {
                ex_status |= check_condititons(curr_list, 0); //выполнение
            }
            if(list[i++] != NULL)
            {
                if(list[i] == NULL)
                {
                    deleteL(curr_list);
                    break;
                }
                else
                {
                    k = i;
                }
            }
            else
            {
                deleteL(curr_list);
                break;
            }
            free(curr_list);
        }
        exit(ex_status);
    }
    if(pid < 0)
    {
        fprintf(stderr, RED_TEXT "ERROR: fork() ret %d\n" RESET_COLOR, pid);
        abort();
    }
    int st;
    waitpid(pid, &st, 0);
    if(WIFEXITED(st) != 0)    // возвращаем 8 младших битов
        return WEXITSTATUS(st);
    else
        return -1;
}

int main()
{
    int ex_status;
    char** user_input;
    while(1)
    {
        printf(GREEN_TEXT "$ " RESET_COLOR);
        user_input = read_user_inp();
        if(user_input != NULL)
        {
            if(strcmp(user_input[0], "exit") == 0)
            {
                deleteL(user_input);
                break;
            }
            ex_status = interpretate_commands(user_input); //отправляем на выполнение
            if(ex_status && (ex_status != SIGINT))
            {
                if(ex_status == -1)
                    fprintf(stderr, "ERROR: invalid command construction\n");
                else
                    fprintf(stderr, "ERROR: command exit status is %d\n", ex_status);
            }
            deleteL(user_input);  // освобождаем память
        }
    }
    return 0;
}
/* cat abc && ls || ps -a > abc ; cat abc */
/* ps j ; (ps j && ls ; pwd); ls */
/* ps -a ; (ps -a && ls && sleep 10) & ps -a */
/* date +%N && ps -a ; (date +%N && ps -a && ls && sleep 10) & date +%N && ps -a */
/* abra schwabra kadabra */
/* ls -lR | grep ^d */
/* ls -l -a /var */
/* sleep 10 & ps -a */
/* ls | wc > bcd ; cat bcd */
/* sleep 1 ; ps -a */
/* false || ls */
/* cat egf || ls */
/* sleep 1 | sleep 5 */
/* sleep 5 | sleep 5 */
/* sleep 5 & */
/* cat test.c | cat | cat | grep int | cat | grep -v main | cat */
1.0021 
1.2321
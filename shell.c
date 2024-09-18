
#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sched.h>

#define MAX_ARGS 100

// Global buffer variables
char *buffer;
size_t buffer_size = 10;

void change_directory(char **args)
{

	if (args[1] == NULL) {
		char error_header[] = "error: ";
		char error_message[] = "must have one argument";

		write(STDOUT_FILENO, error_header, strlen(error_header));
		write(STDOUT_FILENO, error_message, strlen(error_message));
		write(STDOUT_FILENO, "\n", 1);
		return;
	}
	if (args[2] != NULL) {
		char error_header[] = "error: ";
		char error_message[] = "too many arguments";

		write(STDOUT_FILENO, error_header, strlen(error_header));
		write(STDOUT_FILENO, error_message, strlen(error_message));
		write(STDOUT_FILENO, "\n", 1);
		return;
	}

	int result = syscall(SYS_chdir, args[1]);

	if (result == -1) {
		char error_header[] = "error: ";
		char *error_message = strerror(errno);

		write(STDOUT_FILENO, error_header, strlen(error_header));
		write(STDOUT_FILENO, error_message, strlen(error_message));
		write(STDOUT_FILENO, "\n", 1);
	}
}

int child_function(void *input)
{
	// Child function called after clone()
	char **args = (char **)input;
	static const char * const envp[] = {"TERM=xterm", NULL};


	// Signal handling
	struct sigaction act;

	act.sa_handler = SIG_DFL;

	syscall(SYS_rt_sigaction, SIGINT, &act, NULL, 8);


	int result = syscall(SYS_execve, args[0], args, envp);

	if (result == -1) {
		char error_header[] = "error: ";

		char *error_message = strerror(errno);

		write(STDOUT_FILENO, error_header, strlen(error_header));
		write(STDOUT_FILENO, error_message, strlen(error_message));
		write(STDOUT_FILENO, "\n", 1);
		syscall(SYS_exit_group, 1);
	}
	syscall(SYS_exit_group, 0);
	return 0;
}

void execute_command(char **args)
{

	if (strcmp(args[0], "exit") == 0) {
		if (args[1] != NULL) {
			char error_header[] = "error: ";
			char error_message[] =
			"expecting 0 arguments(or any other valid expression)";

			write(STDOUT_FILENO,
			error_header,
			strlen(error_header));

			write(STDOUT_FILENO,
			error_message,
			strlen(error_message));

			write(STDOUT_FILENO, "\n", 1);
		return;
		}
		munmap(buffer, buffer_size * sizeof(char));
		syscall(SYS_exit_group, 0);
	}
	if (strcmp(args[0], "cd") == 0) {
		change_directory(args);
		return;
	}

	// Allocating 1 MB for the child program
	size_t child_stack_size = 1024 * 1024 * sizeof(char);
	char *child_stack = mmap(NULL,
				child_stack_size,
				PROT_READ|PROT_WRITE,
				MAP_ANONYMOUS|MAP_PRIVATE,
				-1, 0);

	pid_t pid = clone(child_function,
			child_stack + child_stack_size,
			SIGCHLD,
			args);

	if (pid == -1) {
		char error_header[] = "error: ";
		char *error_message = strerror(errno);

		write(STDOUT_FILENO,
			error_header,
			strlen(error_header));

		write(STDOUT_FILENO,
			error_message,
			strlen(error_message));

		write(STDOUT_FILENO, "\n", 1);
	} else {
		// Parent process
		// waiting for the child to finish
		int result = syscall(SYS_wait4, pid, NULL, 0, NULL);

		if (result == -1) {
			char error_header[] = "error: ";
			char *error_message = strerror(errno);

			write(STDOUT_FILENO,
				error_header,
				strlen(error_header));

			write(STDOUT_FILENO,
				error_message,
				strlen(error_message));

			write(STDOUT_FILENO, "\n", 1);
		}
	}
	munmap(child_stack, child_stack_size);
}

int main(void)
{
	char *tokens[MAX_ARGS];

	buffer = mmap(NULL,
			buffer_size * sizeof(char),
			PROT_READ|PROT_WRITE,
			MAP_ANONYMOUS|MAP_PRIVATE,
			-1, 0);
	if (buffer == MAP_FAILED) {
		char error_header[] = "error: ";
		char *error_message = strerror(errno);

		write(STDOUT_FILENO, error_header, strlen(error_header));
		write(STDOUT_FILENO, error_message, strlen(error_message));
		write(STDOUT_FILENO, "\n", 1);
	}

	// Signal handling
	struct sigaction act;

	act.sa_handler = SIG_IGN;
	syscall(SYS_rt_sigaction, SIGINT, &act, NULL, 8);

	while (1) {

		write(STDOUT_FILENO, "$", 1);

		// Read input
		int position = 0;

		while (1) {
			ssize_t bytes_read = read(STDIN_FILENO,
							buffer + position,
							1);
			if (bytes_read == -1) {
				char error_header[] = "error: ";
				char *error_message = strerror(errno);

				write(STDOUT_FILENO,
					error_header,
					strlen(error_header));

				write(STDOUT_FILENO,
					error_message,
					strlen(error_message));

				write(STDOUT_FILENO, "\n", 1);
				break;
			}
			if (buffer[position] == '\n')
				break;
			position += 1;

			//Check for buffer overflow and resize if so
			if (position >= buffer_size) {
				size_t old_buffer_size = buffer_size;

				buffer_size *= 2;

				char *new_buffer = mmap(NULL,
							buffer_size *
							sizeof(char),
							PROT_READ|
							PROT_WRITE,
							MAP_ANONYMOUS|
							MAP_PRIVATE,
							-1, 0);

				for (int i = 0; i < position; i++)
					new_buffer[i] = buffer[i];
				munmap(buffer, old_buffer_size * sizeof(char));
				buffer = new_buffer;
			}
		}

		// Mark end of string
		buffer[position] = '\0';

		// Remove newline character
		buffer[strcspn(buffer, "\n")] = 0;

		// Tokenize input
		int token_count = 0;
		char *token = strtok(buffer, " ");

		while (token != NULL && token_count < MAX_ARGS - 1) {
			tokens[token_count++] = token;
			token = strtok(NULL, " ");
		}
		// Add null to end of list to indicate end
		tokens[token_count] = NULL;

		// No command entered
		if (token_count == 0)
			continue;
		execute_command(tokens);
	}

	return 0;
}


/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   microshell.c                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: renebraaksma <renebraaksma@student.42.f      +#+                     */
/*                                                   +#+                      */
/*   Created: 2020/12/28 21:01:46 by rbraaksm      #+#    #+#                 */
/*   Updated: 2020/12/31 13:53:31 by rbraaksm      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct 	s_cmd{
	char	**av;
	char	**ev;
	int		i;
	int		c;
	int		ret;
}				t_cmd;

int		ft_strlen(char *str){
	int i = 0;
	while (str[i])
		i++;
	return (i);
}

int		errors(t_cmd *m, char *error, int index){
	m->ret = EXIT_FAILURE;
	if (index == 0)
		write(2, "error: cd: bad arguments", ft_strlen("error: cd: bad arguments"));
	else if (index == 1){
		write(2, "error: cd: cannot change directory to ", ft_strlen("error: cd: cannot change directory to "));
		write(2, error, ft_strlen(error));
	}
	else if (index == 2)
		write(2, "error: fatal", ft_strlen("error: fatal"));
	else if (index == 3){
		write(2, "error: cannot execute ", ft_strlen("error: cannot execute "));
		write(2, error, ft_strlen(error));
	}
	write(2, "\n", 1);
	return (m->ret);
}

void	ispipe(t_cmd *m){
	int i = m->i;
	m->c = 0;
	while (m->av[i] && strcmp(";", m->av[i])){
		if (strcmp("|", m->av[i]) == 0)
			m->c++;
		i++;
	}
}

void	create_pipe(t_cmd *m, int pipes[], int count, int i){
	if (i == count)
		return ;
	if (pipe(pipes + (i * 2)) < 0)
		errors(m, NULL, 2);
}

char	**create_arg(t_cmd *m){
	char	**new;
	int		index = m->i;
	int 	i = 0;
	while (m->av[m->i] && strcmp(";", m->av[m->i]) && strcmp("|", m->av[m->i])){
		i++;
		m->i++;
	}
	new = malloc(sizeof(char *) * (i + 1));
	if (new == NULL)
		errors(m, NULL, 2);
	int x = 0;
	while (x < i){
		new[x] = m->av[index];
		index++;
		x++;
	}
	new[x] = NULL;
	return (new);
}

void	close_pipe(t_cmd *m, int pipes[], int count, int i){
	if (i < count)
		if (close(pipes[i * 2 + 1]) < 0)
			errors(m, NULL, 2);
	if (i > 0)
		if (close(pipes[(i - 1) * 2]) < 0)
			errors(m, NULL, 2);
}

void	execute(t_cmd *m, char **arg){
	if (execve(arg[0], arg, m->ev) < 0){
		errors(m, arg[0], 3);
		exit(127);
	}
}

void	wait_exit(t_cmd *m, pid_t pid){
	int status = 0;
	if (waitpid(pid, &status, 0) < 0)
		errors(m, NULL, 2);
	if (WIFEXITED(status))
		if ((m->ret = WEXITSTATUS(status)) < 0)
			errors(m, NULL, 2);
}

void	exec_command(t_cmd *m){
	ispipe(m);
	int		pipes[m->c * 2 + 1];
	pid_t	pids[m->c + 1];
	int		i = 0;
	char	**arg;
	while (i < (m->c + 1)){
		if (strcmp("|", m->av[m->i]) == 0)
			m->i++;
		if (m->c)
			create_pipe(m, pipes, m->c, i);
		arg = create_arg(m);
		pids[i] = fork();
		if (pids[i] < 0)
			errors(m, NULL, 2);
		else if (pids[i] == 0){
			if (m->c && i < m->c && dup2(pipes[i * 2 + 1], 1) < 0)
				errors(m, NULL, 2);
			if (m->c && i > 0 && dup2(pipes[(i - 1) * 2], 0) < 0)
				errors(m, NULL, 2);
			if (m->c)
				close_pipe(m, pipes, m->c, i);
			execute(m, arg);
		}
		free(arg);
		if (m->c)
			close_pipe(m, pipes, m->c, i);
		wait_exit(m, pids[i]);
		i++;
	}
}

void	cd_command(t_cmd *m){
	int i = m->i;
	while (m->av[m->i] && strcmp(";", m->av[m->i]) && strcmp("|", m->av[m->i]))
		m->i++;
	if ((m->i - i) < 2)
		errors(m, NULL, 0);
	else if (chdir(m->av[i + 1]) < 0)
		errors(m, m->av[i + 1], 1);
	else
		m->ret = EXIT_SUCCESS;
}

int		main(int ac, char **av, char **ev){
	t_cmd m;
	m.av = av;
	m.ev = ev;
	m.i = 1;
	m.ret = EXIT_SUCCESS;
	while (m.av[m.i]){
		if (strcmp(";", m.av[m.i]) == 0)
			m.i++;
		else if (strcmp("cd", m.av[m.i]) == 0)
			cd_command(&m);
		else
			exec_command(&m);
	}
	return (m.ret);
	(void)ac;
}
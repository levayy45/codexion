/* ************************************************************************** */
/*                                                                            */
/*                                                       :::      ::::::::    */
/*   codexion.h                                        :+:      :+:    :+:    */
/*                                                   +:+ +:+         +:+      */
/*   By: levayy <levayy@student.42.fr>             +#+  +:+       +#+         */
/*                                               +#+#+#+#+#+   +#+            */
/*   Created: 2026/07/20 12:00:00 by levayy           #+#    #+#              */
/*   Updated: 2026/07/20 12:00:00 by levayy          ###   ########.fr        */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <sys/time.h>
# include <time.h>
# include <stdlib.h>
# include <unistd.h>
# include <stdio.h>
# include <string.h>

typedef struct s_req
{
	long	key;
	int		id;
}	t_req;

typedef struct s_heap
{
	t_req	*arr;
	int		size;
}	t_heap;

typedef struct s_dongle
{
	pthread_mutex_t	mtx;
	pthread_cond_t	cond;
	int				held;
	long			ready_at;
	t_heap			queue;
}	t_dongle;

typedef struct s_coder
{
	int				id;
	int				compiles;
	long			last_compile;
	pthread_t		thread;
	pthread_mutex_t	mtx;
	t_dongle		*first;
	t_dongle		*second;
	struct s_sim	*sim;
}	t_coder;

typedef struct s_sim
{
	int				n;
	long			t_burnout;
	long			t_compile;
	long			t_debug;
	long			t_refactor;
	int				must_compile;
	long			cooldown;
	int				edf;
	long			start;
	int				stopped;
	long			seq;
	pthread_mutex_t	stop_mtx;
	pthread_mutex_t	print_mtx;
	pthread_mutex_t	seq_mtx;
	t_dongle		*dongles;
	t_coder			*coders;
	pthread_t		monitor;
}	t_sim;

/* parse.c */
int		parse_args(t_sim *sim, int argc, char **argv);

/* init.c */
int		init_dongles(t_sim *sim);
int		init_coders(t_sim *sim);

/* cleanup.c */
void	destroy_n_dongles(t_sim *sim, int count);
void	destroy_n_coders(t_sim *sim, int count);
void	destroy_sim_mutexes(t_sim *sim);
void	destroy_sim(t_sim *sim);
void	join_threads(t_sim *sim, int count);

/* heap.c */
void	heap_push(t_heap *h, t_req r);
void	heap_pop(t_heap *h);
int		heap_top_id(t_heap *h);

/* time.c */
long	now_ms(void);
long	elapsed_ms(t_sim *sim);
void	smart_sleep(t_sim *sim, long duration);

/* state.c */
int		sim_stopped(t_sim *sim);
void	set_stop(t_sim *sim);
long	next_seq(t_sim *sim);
void	print_state(t_coder *c, const char *msg);
void	print_burnout(t_sim *sim, int id);

/* dongle.c */
int		take_dongle(t_coder *c, t_dongle *d);

/* dongle_utils.c */
void	release_dongle(t_sim *sim, t_dongle *d);
void	wake_all_dongles(t_sim *sim);

/* coder.c */
void	*coder_routine(void *arg);

/* monitor.c */
void	*monitor_routine(void *arg);

#endif

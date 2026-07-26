/* ************************************************************************** */
/*                                                                            */
/*                                                       :::      ::::::::    */
/*   state.c                                           :+:      :+:    :+:    */
/*                                                   +:+ +:+         +:+      */
/*   By: levayy <levayy@student.42.fr>             +#+  +:+       +#+         */
/*                                               +#+#+#+#+#+   +#+            */
/*   Created: 2026/07/20 12:00:00 by levayy           #+#    #+#              */
/*   Updated: 2026/07/20 12:00:00 by levayy          ###   ########.fr        */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	sim_stopped(t_sim *sim)
{
	int	s;

	pthread_mutex_lock(&sim->stop_mtx);
	s = sim->stopped;
	pthread_mutex_unlock(&sim->stop_mtx);
	return (s);
}

void	set_stop(t_sim *sim)
{
	pthread_mutex_lock(&sim->stop_mtx);
	sim->stopped = 1;
	pthread_mutex_unlock(&sim->stop_mtx);
}

long	next_seq(t_sim *sim)
{
	long	s;

	pthread_mutex_lock(&sim->seq_mtx);
	s = sim->seq;
	sim->seq++;
	pthread_mutex_unlock(&sim->seq_mtx);
	return (s);
}

void	print_state(t_coder *c, const char *msg)
{
	pthread_mutex_lock(&c->sim->print_mtx);
	if (!sim_stopped(c->sim))
		printf("%ld %d %s\n", elapsed_ms(c->sim), c->id, msg);
	pthread_mutex_unlock(&c->sim->print_mtx);
}

void	print_burnout(t_sim *sim, int id)
{
	pthread_mutex_lock(&sim->print_mtx);
	printf("%ld %d burned out\n", elapsed_ms(sim), id);
	pthread_mutex_unlock(&sim->print_mtx);
}

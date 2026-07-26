/* ************************************************************************** */
/*                                                                            */
/*                                                       :::      ::::::::    */
/*   cleanup.c                                         :+:      :+:    :+:    */
/*                                                   +:+ +:+         +:+      */
/*   By: levayy <levayy@student.42.fr>             +#+  +:+       +#+         */
/*                                               +#+#+#+#+#+   +#+            */
/*   Created: 2026/07/20 12:00:00 by levayy           #+#    #+#              */
/*   Updated: 2026/07/20 12:00:00 by levayy          ###   ########.fr        */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	destroy_n_dongles(t_sim *sim, int count)
{
	int	i;

	if (!sim->dongles)
		return ;
	i = 0;
	while (i < count)
	{
		pthread_mutex_destroy(&sim->dongles[i].mtx);
		pthread_cond_destroy(&sim->dongles[i].cond);
		free(sim->dongles[i].queue.arr);
		i++;
	}
	free(sim->dongles);
	sim->dongles = NULL;
}

void	destroy_n_coders(t_sim *sim, int count)
{
	int	i;

	if (!sim->coders)
		return ;
	i = 0;
	while (i < count)
	{
		pthread_mutex_destroy(&sim->coders[i].mtx);
		i++;
	}
	free(sim->coders);
	sim->coders = NULL;
}

void	destroy_sim_mutexes(t_sim *sim)
{
	pthread_mutex_destroy(&sim->stop_mtx);
	pthread_mutex_destroy(&sim->print_mtx);
	pthread_mutex_destroy(&sim->seq_mtx);
}

void	destroy_sim(t_sim *sim)
{
	destroy_n_dongles(sim, sim->n);
	destroy_n_coders(sim, sim->n);
	destroy_sim_mutexes(sim);
}

void	join_threads(t_sim *sim, int count)
{
	int	i;

	i = 0;
	while (i < count)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
	pthread_join(sim->monitor, NULL);
}

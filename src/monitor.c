/* ************************************************************************** */
/*                                                                            */
/*                                                       :::      ::::::::    */
/*   monitor.c                                         :+:      :+:    :+:    */
/*                                                   +:+ +:+         +:+      */
/*   By: levayy <levayy@student.42.fr>             +#+  +:+       +#+         */
/*                                               +#+#+#+#+#+   +#+            */
/*   Created: 2026/07/20 12:00:00 by levayy           #+#    #+#              */
/*   Updated: 2026/07/20 12:00:00 by levayy          ###   ########.fr        */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	coder_burned(t_coder *c, t_sim *sim)
{
	long	last;

	pthread_mutex_lock(&c->mtx);
	last = c->last_compile;
	pthread_mutex_unlock(&c->mtx);
	return (now_ms() - last > sim->t_burnout);
}

static int	all_compiled(t_sim *sim)
{
	int	i;
	int	done;

	i = 0;
	while (i < sim->n)
	{
		pthread_mutex_lock(&sim->coders[i].mtx);
		done = (sim->coders[i].compiles >= sim->must_compile);
		pthread_mutex_unlock(&sim->coders[i].mtx);
		if (!done)
			return (0);
		i++;
	}
	return (1);
}

static int	check_coders(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->n)
	{
		if (coder_burned(&sim->coders[i], sim))
		{
			set_stop(sim);
			print_burnout(sim, sim->coders[i].id);
			return (1);
		}
		i++;
	}
	if (all_compiled(sim))
	{
		set_stop(sim);
		return (1);
	}
	return (0);
}

void	*monitor_routine(void *arg)
{
	t_sim	*sim;

	sim = (t_sim *)arg;
	while (!sim_stopped(sim))
	{
		if (check_coders(sim))
		{
			wake_all_dongles(sim);
			return (NULL);
		}
		usleep(500);
	}
	return (NULL);
}

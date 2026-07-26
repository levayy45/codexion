/* ************************************************************************** */
/*                                                                            */
/*                                                       :::      ::::::::    */
/*   main.c                                            :+:      :+:    :+:    */
/*                                                   +:+ +:+         +:+      */
/*   By: levayy <levayy@student.42.fr>             +#+  +:+       +#+         */
/*                                               +#+#+#+#+#+   +#+            */
/*   Created: 2026/07/20 12:00:00 by levayy           #+#    #+#              */
/*   Updated: 2026/07/20 12:00:00 by levayy          ###   ########.fr        */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	init_sim_mutexes(t_sim *sim)
{
	if (pthread_mutex_init(&sim->stop_mtx, NULL) != 0)
		return (0);
	if (pthread_mutex_init(&sim->print_mtx, NULL) != 0)
	{
		pthread_mutex_destroy(&sim->stop_mtx);
		return (0);
	}
	if (pthread_mutex_init(&sim->seq_mtx, NULL) != 0)
	{
		pthread_mutex_destroy(&sim->stop_mtx);
		pthread_mutex_destroy(&sim->print_mtx);
		return (0);
	}
	return (1);
}

static int	init_sim(t_sim *sim)
{
	if (!init_sim_mutexes(sim))
		return (0);
	if (!init_dongles(sim))
	{
		destroy_sim_mutexes(sim);
		return (0);
	}
	if (!init_coders(sim))
	{
		destroy_n_dongles(sim, sim->n);
		destroy_sim_mutexes(sim);
		return (0);
	}
	return (1);
}

static int	start_threads(t_sim *sim)
{
	int	i;

	sim->start = now_ms();
	i = 0;
	while (i < sim->n)
	{
		sim->coders[i].last_compile = sim->start;
		i++;
	}
	if (pthread_create(&sim->monitor, NULL, monitor_routine, sim) != 0)
		return (0);
	i = 0;
	while (i < sim->n)
	{
		if (pthread_create(&sim->coders[i].thread, NULL,
				coder_routine, &sim->coders[i]) != 0)
		{
			set_stop(sim);
			join_threads(sim, i);
			return (0);
		}
		i++;
	}
	return (1);
}

int	main(int argc, char **argv)
{
	t_sim	sim;

	if (!parse_args(&sim, argc, argv))
	{
		fprintf(stderr, "Error: invalid arguments\nUsage: ./codexion "
			"number_of_coders time_to_burnout time_to_compile time_to_debug "
			"time_to_refactor number_of_compiles_required dongle_cooldown "
			"scheduler(fifo|edf)\n");
		return (1);
	}
	if (!init_sim(&sim))
	{
		fprintf(stderr, "Error: initialization failed\n");
		return (1);
	}
	if (!start_threads(&sim))
	{
		destroy_sim(&sim);
		fprintf(stderr, "Error: thread creation failed\n");
		return (1);
	}
	join_threads(&sim, sim.n);
	destroy_sim(&sim);
	return (0);
}

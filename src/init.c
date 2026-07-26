/* ************************************************************************** */
/*                                                                            */
/*                                                       :::      ::::::::    */
/*   init.c                                            :+:      :+:    :+:    */
/*                                                   +:+ +:+         +:+      */
/*   By: levayy <levayy@student.42.fr>             +#+  +:+       +#+         */
/*                                               +#+#+#+#+#+   +#+            */
/*   Created: 2026/07/20 12:00:00 by levayy           #+#    #+#              */
/*   Updated: 2026/07/20 12:00:00 by levayy          ###   ########.fr        */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	init_dongle(t_dongle *d, int n)
{
	d->held = 0;
	d->ready_at = 0;
	d->queue.size = 0;
	d->queue.arr = malloc(sizeof(t_req) * n);
	if (!d->queue.arr)
		return (0);
	if (pthread_mutex_init(&d->mtx, NULL) != 0)
	{
		free(d->queue.arr);
		d->queue.arr = NULL;
		return (0);
	}
	if (pthread_cond_init(&d->cond, NULL) != 0)
	{
		pthread_mutex_destroy(&d->mtx);
		free(d->queue.arr);
		d->queue.arr = NULL;
		return (0);
	}
	return (1);
}

int	init_dongles(t_sim *sim)
{
	int	i;

	sim->dongles = malloc(sizeof(t_dongle) * sim->n);
	if (!sim->dongles)
		return (0);
	memset(sim->dongles, 0, sizeof(t_dongle) * sim->n);
	i = 0;
	while (i < sim->n)
	{
		if (!init_dongle(&sim->dongles[i], sim->n))
		{
			destroy_n_dongles(sim, i);
			return (0);
		}
		i++;
	}
	return (1);
}

static void	assign_dongles(t_coder *c)
{
	t_dongle	*left;
	t_dongle	*right;

	left = &c->sim->dongles[c->id - 1];
	right = &c->sim->dongles[c->id % c->sim->n];
	if (left <= right)
	{
		c->first = left;
		c->second = right;
	}
	else
	{
		c->first = right;
		c->second = left;
	}
}

int	init_coders(t_sim *sim)
{
	int	i;

	sim->coders = malloc(sizeof(t_coder) * sim->n);
	if (!sim->coders)
		return (0);
	memset(sim->coders, 0, sizeof(t_coder) * sim->n);
	i = 0;
	while (i < sim->n)
	{
		sim->coders[i].id = i + 1;
		sim->coders[i].sim = sim;
		if (pthread_mutex_init(&sim->coders[i].mtx, NULL) != 0)
		{
			destroy_n_coders(sim, i);
			return (0);
		}
		assign_dongles(&sim->coders[i]);
		i++;
	}
	return (1);
}

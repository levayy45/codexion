/* ************************************************************************** */
/*                                                                            */
/*                                                       :::      ::::::::    */
/*   dongle_utils.c                                    :+:      :+:    :+:    */
/*                                                   +:+ +:+         +:+      */
/*   By: levayy <levayy@student.42.fr>             +#+  +:+       +#+         */
/*                                               +#+#+#+#+#+   +#+            */
/*   Created: 2026/07/20 12:00:00 by levayy           #+#    #+#              */
/*   Updated: 2026/07/20 12:00:00 by levayy          ###   ########.fr        */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	release_dongle(t_sim *sim, t_dongle *d)
{
	pthread_mutex_lock(&d->mtx);
	d->held = 0;
	d->ready_at = now_ms() + sim->cooldown;
	pthread_cond_broadcast(&d->cond);
	pthread_mutex_unlock(&d->mtx);
}

void	wake_all_dongles(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->n)
	{
		pthread_mutex_lock(&sim->dongles[i].mtx);
		pthread_cond_broadcast(&sim->dongles[i].cond);
		pthread_mutex_unlock(&sim->dongles[i].mtx);
		i++;
	}
}

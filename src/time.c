/* ************************************************************************** */
/*                                                                            */
/*                                                       :::      ::::::::    */
/*   time.c                                            :+:      :+:    :+:    */
/*                                                   +:+ +:+         +:+      */
/*   By: levayy <levayy@student.42.fr>             +#+  +:+       +#+         */
/*                                               +#+#+#+#+#+   +#+            */
/*   Created: 2026/07/20 12:00:00 by levayy           #+#    #+#              */
/*   Updated: 2026/07/20 12:00:00 by levayy          ###   ########.fr        */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long	now_ms(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000L + tv.tv_usec / 1000);
}

long	elapsed_ms(t_sim *sim)
{
	return (now_ms() - sim->start);
}

void	smart_sleep(t_sim *sim, long duration)
{
	long	end;

	end = now_ms() + duration;
	while (!sim_stopped(sim) && now_ms() < end)
		usleep(200);
}

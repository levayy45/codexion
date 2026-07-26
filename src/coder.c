/* ************************************************************************** */
/*                                                                            */
/*                                                       :::      ::::::::    */
/*   coder.c                                           :+:      :+:    :+:    */
/*                                                   +:+ +:+         +:+      */
/*   By: levayy <levayy@student.42.fr>             +#+  +:+       +#+         */
/*                                               +#+#+#+#+#+   +#+            */
/*   Created: 2026/07/20 12:00:00 by levayy           #+#    #+#              */
/*   Updated: 2026/07/20 12:00:00 by levayy          ###   ########.fr        */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	lone_coder(t_coder *c)
{
	if (!take_dongle(c, c->first))
		return ;
	while (!sim_stopped(c->sim))
		usleep(200);
}

static int	do_compile(t_coder *c)
{
	if (!take_dongle(c, c->first))
		return (0);
	if (!take_dongle(c, c->second))
	{
		release_dongle(c->sim, c->first);
		return (0);
	}
	pthread_mutex_lock(&c->mtx);
	c->last_compile = now_ms();
	c->compiles++;
	pthread_mutex_unlock(&c->mtx);
	print_state(c, "is compiling");
	smart_sleep(c->sim, c->sim->t_compile);
	release_dongle(c->sim, c->first);
	release_dongle(c->sim, c->second);
	return (1);
}

void	*coder_routine(void *arg)
{
	t_coder	*c;

	c = (t_coder *)arg;
	if (c->first == c->second)
	{
		lone_coder(c);
		return (NULL);
	}
	while (!sim_stopped(c->sim))
	{
		if (!do_compile(c))
			break ;
		print_state(c, "is debugging");
		smart_sleep(c->sim, c->sim->t_debug);
		print_state(c, "is refactoring");
		smart_sleep(c->sim, c->sim->t_refactor);
	}
	return (NULL);
}

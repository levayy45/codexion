/* ************************************************************************** */
/*                                                                            */
/*                                                       :::      ::::::::    */
/*   dongle.c                                          :+:      :+:    :+:    */
/*                                                   +:+ +:+         +:+      */
/*   By: levayy <levayy@student.42.fr>             +#+  +:+       +#+         */
/*                                               +#+#+#+#+#+   +#+            */
/*   Created: 2026/07/20 12:00:00 by levayy           #+#    #+#              */
/*   Updated: 2026/07/20 12:00:00 by levayy          ###   ########.fr        */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static long	deadline_of(t_coder *c)
{
	long	d;

	pthread_mutex_lock(&c->mtx);
	d = c->last_compile + c->sim->t_burnout;
	pthread_mutex_unlock(&c->mtx);
	return (d);
}

static t_req	make_req(t_coder *c)
{
	t_req	r;

	if (c->sim->edf)
		r.key = deadline_of(c);
	else
		r.key = next_seq(c->sim);
	r.id = c->id;
	return (r);
}

static int	can_take(t_coder *c, t_dongle *d)
{
	if (d->held)
		return (0);
	if (heap_top_id(&d->queue) != c->id)
		return (0);
	if (now_ms() < d->ready_at)
		return (0);
	return (1);
}

static void	wait_turn(t_coder *c, t_dongle *d)
{
	struct timespec	ts;

	while (!sim_stopped(c->sim) && !can_take(c, d))
	{
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_nsec += 1000000;
		if (ts.tv_nsec >= 1000000000)
		{
			ts.tv_sec++;
			ts.tv_nsec -= 1000000000;
		}
		pthread_cond_timedwait(&d->cond, &d->mtx, &ts);
	}
}

int	take_dongle(t_coder *c, t_dongle *d)
{
	pthread_mutex_lock(&d->mtx);
	heap_push(&d->queue, make_req(c));
	wait_turn(c, d);
	if (sim_stopped(c->sim))
	{
		pthread_mutex_unlock(&d->mtx);
		return (0);
	}
	heap_pop(&d->queue);
	d->held = 1;
	pthread_mutex_unlock(&d->mtx);
	print_state(c, "has taken a dongle");
	return (1);
}

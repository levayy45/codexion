/* ************************************************************************** */
/*                                                                            */
/*                                                       :::      ::::::::    */
/*   parse.c                                           :+:      :+:    :+:    */
/*                                                   +:+ +:+         +:+      */
/*   By: levayy <levayy@student.42.fr>             +#+  +:+       +#+         */
/*                                               +#+#+#+#+#+   +#+            */
/*   Created: 2026/07/20 12:00:00 by levayy           #+#    #+#              */
/*   Updated: 2026/07/20 12:00:00 by levayy          ###   ########.fr        */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static long	parse_pos(const char *s)
{
	long	val;
	int		i;

	if (!s || !s[0])
		return (-1);
	i = 0;
	if (s[i] == '+')
		i++;
	if (!s[i])
		return (-1);
	val = 0;
	while (s[i])
	{
		if (s[i] < '0' || s[i] > '9')
			return (-1);
		val = val * 10 + (s[i] - '0');
		if (val > 2147483647)
			return (-1);
		i++;
	}
	return (val);
}

static int	parse_scheduler(t_sim *sim, const char *s)
{
	if (strcmp(s, "fifo") == 0)
		sim->edf = 0;
	else if (strcmp(s, "edf") == 0)
		sim->edf = 1;
	else
		return (0);
	return (1);
}

int	parse_args(t_sim *sim, int argc, char **argv)
{
	memset(sim, 0, sizeof(t_sim));
	if (argc != 9)
		return (0);
	sim->n = (int)parse_pos(argv[1]);
	sim->t_burnout = parse_pos(argv[2]);
	sim->t_compile = parse_pos(argv[3]);
	sim->t_debug = parse_pos(argv[4]);
	sim->t_refactor = parse_pos(argv[5]);
	sim->must_compile = (int)parse_pos(argv[6]);
	sim->cooldown = parse_pos(argv[7]);
	if (sim->n < 1 || sim->t_burnout < 0 || sim->t_compile < 0
		|| sim->t_debug < 0 || sim->t_refactor < 0
		|| sim->must_compile < 1 || sim->cooldown < 0)
		return (0);
	return (parse_scheduler(sim, argv[8]));
}

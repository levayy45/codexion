/* ************************************************************************** */
/*                                                                            */
/*                                                       :::      ::::::::    */
/*   heap.c                                            :+:      :+:    :+:    */
/*                                                   +:+ +:+         +:+      */
/*   By: levayy <levayy@student.42.fr>             +#+  +:+       +#+         */
/*                                               +#+#+#+#+#+   +#+            */
/*   Created: 2026/07/20 12:00:00 by levayy           #+#    #+#              */
/*   Updated: 2026/07/20 12:00:00 by levayy          ###   ########.fr        */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	heap_less(t_req a, t_req b)
{
	if (a.key != b.key)
		return (a.key < b.key);
	return (a.id < b.id);
}

static void	heap_swap(t_req *a, t_req *b)
{
	t_req	tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

void	heap_push(t_heap *h, t_req r)
{
	int	i;

	h->arr[h->size] = r;
	i = h->size;
	h->size++;
	while (i > 0 && heap_less(h->arr[i], h->arr[(i - 1) / 2]))
	{
		heap_swap(&h->arr[i], &h->arr[(i - 1) / 2]);
		i = (i - 1) / 2;
	}
}

void	heap_pop(t_heap *h)
{
	int	i;
	int	child;

	h->size--;
	h->arr[0] = h->arr[h->size];
	i = 0;
	while (2 * i + 1 < h->size)
	{
		child = 2 * i + 1;
		if (child + 1 < h->size
			&& heap_less(h->arr[child + 1], h->arr[child]))
			child++;
		if (!heap_less(h->arr[child], h->arr[i]))
			break ;
		heap_swap(&h->arr[i], &h->arr[child]);
		i = child;
	}
}

int	heap_top_id(t_heap *h)
{
	if (h->size == 0)
		return (-1);
	return (h->arr[0].id);
}

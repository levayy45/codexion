NAME		= codexion

CC			= cc
CFLAGS		= -Wall -Wextra -Werror -pthread
INC			= -Iincludes

SRC_DIR		= src
OBJ_DIR		= obj

SRCS		= main.c parse.c init.c cleanup.c heap.c time.c state.c \
			  dongle.c dongle_utils.c coder.c monitor.c
OBJS		= $(addprefix $(OBJ_DIR)/, $(SRCS:.c=.o))
HDR			= includes/codexion.h

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HDR)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

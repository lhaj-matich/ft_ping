NAME		= ft_ping

CC		= cc
CFLAGS		= -Wall -Wextra -Werror
RM		= rm -f

SRCS		= main.c helper.c print.c icmp.c parser.c list.c
OBJS		= $(SRCS:.c=.o)

all:		$(NAME)

$(NAME):	$(OBJS)
		$(CC) $(CFLAGS) -o $(NAME) $(OBJS) -lm
		$(RM) $(OBJS)

%.o:		%.c header.h
		$(CC) $(CFLAGS) -c $< -o $@

clean:
		$(RM) $(OBJS)

fclean:		clean
		$(RM) $(NAME)

re:		fclean all

.PHONY:		all clean fclean re

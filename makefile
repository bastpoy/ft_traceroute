SRCSPRINTF = main.c
OBJSPRINTF = $(patsubst %.c, objects/%.o, $(SRCSPRINTF))

CFLAGS = -Wall -Werror -Wextra -g3

NAME = ft_traceroute

$(NAME) : include/libft/libft.a $(OBJSPRINTF)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJSPRINTF) ./include/libft/libft.a
all : $(NAME)

include/libft/libft.a: FORCE
	$(MAKE) all -C ./include/libft

objects/%.o : %.c include/libft/libft.h
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean : 
	$(MAKE) clean -C ./include/libft
	rm -rf objects
	rm -rf test

fclean : clean
	rm -f $(NAME)

re : fclean all

FORCE:

.PHONY:  all clean fclean re
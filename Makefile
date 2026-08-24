
NAME = webserv

SRCS = main.cpp \
	Autoindex.cpp CgiHandler.cpp Client.cpp \
	ConfigLoc.cpp ConfigParser.cpp ConfigServ.cpp \
	HttpRequest.cpp HttpResponse.cpp RequestRouter.cpp \
	ServerManager.cpp
OBJS = $(SRCS:.cpp=.o)

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
RM = rm -f

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re

SERVER_NAME = server1
CLIENT_NAME = client1

SERVER_SRCS = test.cpp
CLIENT_SRCS = testclient.cpp

# Object Files
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

# Compiler Settings
CXX      = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
RM       = rm -f

# Default rule (compiles both)
all: $(SERVER_NAME) $(CLIENT_NAME)

# Rule for Server
$(SERVER_NAME): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(SERVER_NAME) $(SERVER_OBJS)

# Rule for Client
$(CLIENT_NAME): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_NAME) $(CLIENT_OBJS)

# Pattern rule for all .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Explicit target aliases
server: $(SERVER_NAME)
client: $(CLIENT_NAME)

clean:
	$(RM) $(SERVER_OBJS) $(CLIENT_OBJS)

fclean: clean
	$(RM) $(SERVER_NAME) $(CLIENT_NAME)

re: fclean all

.PHONY: all clean fclean re server client
NAME = http_flow_test
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
INCLUDES = -I../../includes

SRC_MAIN = main.cpp
SRC_PARSER = ../../srcs/http/HttpRequestParser.cpp
SRC_RESPONSE = ../../srcs/http/HttpResponse.cpp
SRC_FACTORY = ../../srcs/http/HttpHandlerFactory.cpp
SRC_GET = ../../srcs/http/GetHttpHandler.cpp
SRC_POST = ../../srcs/http/PostHttpHandler.cpp
SRC_DELETE = ../../srcs/http/DeleteHttpHandler.cpp

OBJ_MAIN = main.o
OBJ_PARSER = HttpRequestParser.o
OBJ_RESPONSE = HttpResponse.o
OBJ_FACTORY = HttpHandlerFactory.o
OBJ_GET = GetHttpHandler.o
OBJ_POST = PostHttpHandler.o
OBJ_DELETE = DeleteHttpHandler.o

OBJS = $(OBJ_MAIN) $(OBJ_PARSER) $(OBJ_RESPONSE) $(OBJ_FACTORY) $(OBJ_GET) $(OBJ_POST) $(OBJ_DELETE)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(NAME) $(OBJS)

$(OBJ_MAIN): $(SRC_MAIN)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_MAIN) -o $(OBJ_MAIN)

$(OBJ_PARSER): $(SRC_PARSER)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_PARSER) -o $(OBJ_PARSER)

$(OBJ_RESPONSE): $(SRC_RESPONSE)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_RESPONSE) -o $(OBJ_RESPONSE)

$(OBJ_FACTORY): $(SRC_FACTORY)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_FACTORY) -o $(OBJ_FACTORY)

$(OBJ_GET): $(SRC_GET)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_GET) -o $(OBJ_GET)

$(OBJ_POST): $(SRC_POST)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_POST) -o $(OBJ_POST)

$(OBJ_DELETE): $(SRC_DELETE)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_DELETE) -o $(OBJ_DELETE)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

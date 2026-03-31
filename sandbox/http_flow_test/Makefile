NAME = http_flow_test
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
INCLUDES = -I../../includes

SRC_MAIN = main.cpp
SRC_PARSER = ../../srcs/http/HttpRequest.cpp
SRC_RESPONSE = ../../srcs/http/HttpResponse.cpp
SRC_IHANDLER = ../../srcs/http/IHttpHandler.cpp
SRC_GET = ../../srcs/http/GetHttpHandler.cpp
SRC_POST = ../../srcs/http/PostHttpHandler.cpp
SRC_DELETE = ../../srcs/http/DeleteHttpHandler.cpp
SRC_SERVER_CONFIG = ../../srcs/config/ServerConfig.cpp
SRC_LOCATION_CONFIG = ../../srcs/config/LocationConfig.cpp

OBJ_MAIN = main.o
OBJ_PARSER = HttpRequest.o
OBJ_RESPONSE = HttpResponse.o
OBJ_IHANDLER = IHttpHandler.o
OBJ_GET = GetHttpHandler.o
OBJ_POST = PostHttpHandler.o
OBJ_DELETE = DeleteHttpHandler.o
OBJ_SERVER_CONFIG = ServerConfig.o
OBJ_LOCATION_CONFIG = LocationConfig.o

OBJS = $(OBJ_MAIN) $(OBJ_PARSER) $(OBJ_RESPONSE) $(OBJ_IHANDLER) $(OBJ_GET) $(OBJ_POST) $(OBJ_DELETE) $(OBJ_SERVER_CONFIG) $(OBJ_LOCATION_CONFIG)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(NAME) $(OBJS)

$(OBJ_MAIN): $(SRC_MAIN)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_MAIN) -o $(OBJ_MAIN)

$(OBJ_PARSER): $(SRC_PARSER)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_PARSER) -o $(OBJ_PARSER)

$(OBJ_RESPONSE): $(SRC_RESPONSE)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_RESPONSE) -o $(OBJ_RESPONSE)

$(OBJ_IHANDLER): $(SRC_IHANDLER)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_IHANDLER) -o $(OBJ_IHANDLER)

$(OBJ_GET): $(SRC_GET)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_GET) -o $(OBJ_GET)

$(OBJ_POST): $(SRC_POST)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_POST) -o $(OBJ_POST)

$(OBJ_DELETE): $(SRC_DELETE)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_DELETE) -o $(OBJ_DELETE)

$(OBJ_SERVER_CONFIG): $(SRC_SERVER_CONFIG)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_SERVER_CONFIG) -o $(OBJ_SERVER_CONFIG)

$(OBJ_LOCATION_CONFIG): $(SRC_LOCATION_CONFIG)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SRC_LOCATION_CONFIG) -o $(OBJ_LOCATION_CONFIG)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

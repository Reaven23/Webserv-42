# Colors
RESET			= "\033[0m"
BLACK    		= "\033[30m"    # Black
RED      		= "\033[31m"    # Red
GREEN    		= "\033[32m"    # Green
YELLOW   		= "\033[33m"    # Yellow
BLUE     		= "\033[34m"    # Blue
MAGENTA  		= "\033[35m"    # Magenta
CYAN     		= "\033[36m"    # Cyan
WHITE    		= "\033[37m"    # White

# Compiler
NAME			= webserv
CC				= c++
CFLAGS		= -Wall -Wextra -Werror -std=c++98
OS				= $(shell uname)

# Includes
INCLUDES_DIR		= includes
INCLUDES_FLAGS	= -I$(INCLUDES_DIR)
INCLUDES				= $(shell find $(INCLUDES_DIR) -name '*.hpp')

# Sources
SRCS_DIR			= srcs/
SRCS_FILES		= main.cpp \
								CGI/CGI.cpp \
								CGI/getters.cpp \
								CGI/private.cpp \
								CGI/public.cpp \
								CGI/setters.cpp \
								config/Config.cpp \
								config/LocationConfig.cpp \
								config/ServerConfig.cpp \
								network/Client.cpp \
								network/Server.cpp \
								network/ServerSocket.cpp \
								network/helpers.cpp \
								http/CookieUtils.cpp \
								http/DeleteHttpHandler.cpp \
								http/IHttpHandler.cpp \
								http/GetHttpHandler.cpp \
								http/HttpResponse.cpp \
								http/HttpRequest.cpp \
								http/PostHttpHandler.cpp \
								http/SessionHandler.cpp \
								http/SessionManager.cpp \
								utils/Logger.cpp \
								utils/Time.cpp \
								utils/Signals.cpp \
								Webserv.cpp
SRCS					= $(addprefix $(SRCS_DIR), $(SRCS_FILES))

# Objects
OBJS_DIR			= objs/
OBJS_FILES		= $(SRCS_FILES:.cpp=.o)
OBJS					= $(addprefix $(OBJS_DIR), $(OBJS_FILES))

all: $(OBJS_DIR) ${NAME}

$(OBJS_DIR)%.o:	$(SRCS_DIR)%.cpp $(INCLUDES) | $(OBJS_DIR)
	@mkdir -p $(dir $@)
	@${CC} $(CFLAGS) $(INCLUDES_FLAGS) -c $< -o $@

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR) $(addprefix $(OBJS_DIR), $(dir $(SRCS_FILES)))

$(NAME): $(OBJS)
	@echo "compiling $(NAME)..."
	@${CC} ${CFLAGS} ${OBJS} -o ${NAME}
	@echo $(GREEN)"$(NAME) compiled" $(RESET)

clean:
	@echo "deleting objects directory..."
	@rm -rf $(OBJS_DIR)
	@echo $(GREEN) "objects deleted" $(RESET)

fclean: clean
	@echo "begining full clean..."
	@echo " deleting $(NAME)..."
	@rm -f $(NAME)
	@echo $(GREEN) " $(NAME) deleted" $(RESET)
	@echo $(GREEN) "full clean done" $(RESET)

tests:
	@echo "Starting tests suites..."
	@node sandbox/e2e/index.js
	@echo "\n...all tests done."

tests-all:
	@echo "Starting tests suites with slow (keep-alive, timeout)..."
	@node sandbox/e2e/index.js --all
	@echo "\n...all tests done."


re:	fclean all

.PHONY: all clean fclean re

CLIENT_DIR = client
SERVER_DIR = server
COMMON_DIR = common

c:
	make -C $(COMMON_DIR);
	make -C $(CLIENT_DIR);

cclean:
	make clean -C $(COMMON_DIR);
	make clean -C $(CLIENT_DIR);

crun:
	make -C $(COMMON_DIR);
	make run -C $(CLIENT_DIR);

s:
	make -C $(COMMON_DIR);
	make -C $(SERVER_DIR);

sclean:
	make clean -C $(COMMON_DIR);
	make clean -C $(SERVER_DIR);

srun:
	make -C $(COMMON_DIR);
	make run -C $(SERVER_DIR);

cmn:
	make -C $(COMMON_DIR);

cmn_clean:
	make clean -C $(COMMON_DIR);

clean:
	make clean -C $(COMMON_DIR);
	make clean -C $(CLIENT_DIR);
	make clean -C $(SERVER_DIR);
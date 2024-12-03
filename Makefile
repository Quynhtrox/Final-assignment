PRJ_NAME = SenSor_System
FIFO_FILE = logFIFO
LOG_FILE = gateway.log

all:
	gcc -o $(PRJ_NAME) main.c

clean:
	rm -rf $(PRJ_NAME)
	rm -rf $(FIFO_FILE)
	rm -rf $(LOG_FILE)
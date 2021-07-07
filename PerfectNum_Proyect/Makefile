ifeq (run,$(firstword $(MAKECMDGOALS)))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(RUN_ARGS):;@:)
  PROCESSES := $(wordlist 1,1,$(RUN_ARGS))
  NUMBER := $(wordlist 2,2,$(RUN_ARGS))
endif




.PHONY: perfnum
perfnum: main.c
	mpicc main.c -o perfnum -lm


.PHONY: run
run: perfnum
	mpirun -np $(PROCESSES) --oversubscribe perfnum $(NUMBER)


.PHONY: times
times: perfnum
	tiempos.sh
	

.PHONY: clean
clean:
	rm perfnum
	rm tiempos.txt




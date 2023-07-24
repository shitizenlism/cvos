all: $(OBJ) env

%.o: %.cpp
	$(CXX) -c $< $(INC) $(MACRO)

%.o: %.c
	$(CC) -c $< $(INC) $(MACRO)

%: %.cpp
	$(CXX) -o $@ $< $(INC) $(LIB) $(MACRO) -DSMAIN

%: %.c
	$(CXX) -o $@ $< $(INC) $(LIB) $(MACRO) -DSMAIN

env:
	@echo "[32m>>>> compiling under $(TGT) ....[37m"
	#@echo ">>>> compiling under $(TGT)"
clean:
	rm -f *.o $(OBJ)

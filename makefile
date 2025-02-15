CC   =/opt/intel/oneapi/mpi/latest//bin/mpicc
CXX  =/opt/intel/oneapi/mpi/latest//bin/mpicxx


LIB_SuperLU =/home/xtj/MT2/zhengxy/MT3DAnisoCond+Mag/lib/libsuperlu_dist.a
LIB_OpenBLAS=/home/xtj/MT2/zhengxy/MT3DAnisoCond+Mag/lib/libopenblas.a
LIB_parmetis=/home/xtj/MT2/zhengxy/MT3DAnisoCond+Mag/lib/libparmetis.a
LIB_metis   =/home/xtj/MT2/zhengxy/MT3DAnisoCond+Mag/lib/libmetis.a
3D_1A_DIR =/home/xtj/MT2/zhengxy/MT3DAnisoCond+Mag/example/COMMD-1A
1D_DIR =/home/xtj/MT2/zhengxy/MT3DAnisoCond+Mag/example/1D
DIR =/home/xtj/MT2/zhengxy/MT3DAnisoCond+Mag
#LIB_mpi     =/vol7/home/test653/zxx/mpi3-gcc/lib/libmpi.a \
             /vol7/home/test653/zxx/mpi3-gcc/lib/libmpicxx.a

INCLUDE =-I./include
CFLAGES = -c -g -O2 -fopenmp 
LDFLAGES= -g -O2 -fopenmp -o

target=MTSZ
#obj   =main.o prepocess.o global_varible.o post_prepocess.o \
       FEM_compute.o para.o solver.o
obj  = *.o
#src   =main.c prepocess.c global_varible.c post_prepocess.c \
       FEM_compute.c para.c solver.c
src   = *.c
$(target):$(obj) $(LIB_SuperLU) 
#$(LIB_OpenBLAS) $(LIB_parmetis)$(LIB_metis) $(LIB_mpi)
	$(CXX) $(LDFLAGES) $(target) $(obj) $(LIB_SuperLU) $(LIB_OpenBLAS) \
	$(LIB_parmetis) $(LIB_metis) -lm

$(obj):$(src)
	$(CC) $(CFLAGES) $(src) $(INCLUDE)

clean: 
	rm $(obj) $(target)

1D_clean:
	rm -f $(1D_DIR)/$(target)

copy1D:
	cp $(target) $(1D_DIR)

#include "FEM_compute.h"

//-------------------------------------------------------------------------------------------
// 1.Elem_analyse();
// 2.Elem_assemble();
// 3.Process_divide();
// 4.Matrix_Deform();
// 5.Generate_SLU_NR();
//-------------------------------------------------------------------------------------------

void Elem_compute()
{
     if (myrank==0)
     {
          printf("****************************************\n");
          printf("           Elem compute start           \n"); 
          printf("****************************************\n");
     }

     MPI_Barrier(MPI_COMM_WORLD);

     if (myrank==0) time1=MPI_Wtime();

     // --------------Elem_analyse---------------------//
     Elem_analyse();

     // --------------Elem_assemble--------------------//
     Elem_assemble(); 

     // --------------MPI_Fre_Generate-----------------//
     Process_divide();
     
     // --------------Matrix_Deform--------------------//
     if (bound_mode==1)
     {
          Matrix_Deform();
     }


     // --------------Generate_SLU_NR------------------//
     Generate_SLU_NR();

     
     MPI_Barrier(MPI_COMM_WORLD); 
     if (myrank==0)
     {
          time2=MPI_Wtime();
          Fem_compute_time=time2-time1;
     }

     if (myrank==0)
     {
          printf("****************************************\n");
          printf("            Elem compute end            \n"); 
          printf("****************************************\n");
     }
}
//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------
// Elem analyse
//-------------------------------------------------------------------------------------------
void Elem_analyse()
{
     int i;
     int index_elem;
     int index_item;
     int temp;
     int ind_1,ind_2,ind_3,ind_4;
     int length;

     //added by zhengxy
     int j,k,areac,aream;
     int indexc = -1;
     int indexm = -1;
     int Anumbc,Anumbm;
     double loc_mu;
     double sigma1,sigma2,sigma3;
     double alphS,alphD,alphL;
     double sigmaxx, sigmayy, sigmazz;
     double sigmaxy, sigmaxz, sigmayz;
     double sigmayx, sigmazx, sigmazy;
     double Matrixmu[3][3],MatrixV[3][3];
     double mag1,mag2,mag3;
     double alphmS,alphmD,alphmL;
     double magxx, magyy, magzz;
     double magxy, magxz, magyz;
     double magyx, magzx, magzy;
     double vxx, vyy, vzz;
     double vxy, vxz, vyz;
     double vyx, vzx, vzy;

     double cond;
     double x1,x2,x3,x4;
     double y1,y2,y3,y4;
     double z1,z2,z3,z4;
     double Ve; 
     double a1,a2,a3,a4;
     double b1,b2,b3,b4;
     double c1,c2,c3,c4;
     double d1,d2,d3,d4;
     double l1,l2,l3,l4,l5,l6;
     double f11,f12,f13,f14;
     double f22,f23,f24;
     double f33,f34,f44;

     double E_r,E_i;

     length= item_num;
     matrix_complex_real=(double*) malloc( (length)*sizeof(double) );
     matrix_complex_imag=(double*) malloc( (length)*sizeof(double) );
     
     for (i=0;i<=length-1;i++)
     {
         matrix_complex_real[i]=0.0;
     }

     for (i=0;i<=length-1;i++)
     {
         matrix_complex_imag[i]=0.0;
     }

     #pragma omp parallel for schedule(dynamic) \
     private(i,temp,index_elem,index_item,cond,ind_1,ind_2,ind_3,ind_4,x1,x2,x3,x4,\
     y1,y2,y3,y4,z1,z2,z3,z4,Ve,b1,b2,b3,b4,c1,c2,c3,c4,d1,d2,d3,d4,l1,l2,l3,l4,l5,l6,\
     f11,f12,f13,f14,f22,f23,f24,f33,f34,f44,E_r,E_i,j,k,areac,aream,loc_mu,indexc,indexm,Anumbc,\
     Anumbm,sigma1,sigma2,sigma3,alphS,alphD,alphL,sigmaxx,sigmayy,sigmazz,sigmaxy,sigmaxz,sigmayz,\
     sigmayx,sigmazx,sigmazy,Matrixmu,MatrixV,mag1,mag2,mag3,alphmS,alphmD,alphmL,magxx,magyy,magzz,\
     magxy,magxz,magyz,magyx,magzx,magzy,vxx,vyy,vzz,vxy,vxz,vyz,vyx,vzx,vzy) 
     for (i=0;i<=length-1;i++)
     {
          temp=matrix_item_loc[i];
          index_elem=temp/36+1;
          index_item=temp%36;

          if(anisoc.AnisoNumber>0)
          {
               indexc = -1;
               Anumbc = anisoc.AnisoNumber;
               for(j=1;j<=Anumbc;j++)
               {
                    areac = anisoc.areaindex[j];
                    if(areac==elem_pythsical[index_elem])
                    indexc = j;//只要indexc不等于-1，则表示该单元为电导率各向异性单元
               }
          }

          if(anisom.AnisoNumber>0)
          {
               indexm = -1;
               Anumbm = anisom.AnisoNumber;
               for(k=1;k<=Anumbm;k++)
               {
                    aream = anisom.areaindex[k];
                    if(aream==elem_pythsical[index_elem])
                    indexm = k;//只要indexm不等于-1，则表示该单元为磁导率各向异性单元
               }
          }
        
          if(indexc==-1)
          cond = phy_cond[elem_pythsical[index_elem]];
          if(indexc!=-1)
          {
               sigma1 = anisoc.value[3*(indexc-1)+1];
               sigma2 = anisoc.value[3*(indexc-1)+2];//主轴电导率
               sigma3 = anisoc.value[3*(indexc-1)+3];

               alphS = anisoc.angle[3*(indexc-1)+1]*pi/180.0;
               alphD = anisoc.angle[3*(indexc-1)+2]*pi/180.0;//欧拉旋转角
               alphL = anisoc.angle[3*(indexc-1)+3]*pi/180.0;

               sigmaxx = (pow(cos(alphL),2)*sigma1+pow(sin(alphL),2)*sigma2)*pow(cos(alphS),2)+
                           (pow(sin(alphL),2)*sigma1+pow(cos(alphL),2)*sigma2)*pow(sin(alphS),2)*pow(cos(alphD),2)-
                           2.0*(sigma1-sigma2)*sin(alphL)*cos(alphL)*cos(alphD)*sin(alphS)*cos(alphS)+
                           pow(sin(alphS),2)*pow(sin(alphD),2)*sigma3;

               sigmayy = (pow(cos(alphL),2)*sigma1+pow(sin(alphL),2)*sigma2)*pow(sin(alphS),2)+
                         (pow(sin(alphL),2)*sigma1+pow(cos(alphL),2)*sigma2)*pow(cos(alphS),2)*pow(cos(alphD),2)+
                         2.0*(sigma1-sigma2)*sin(alphL)*cos(alphL)*cos(alphD)*sin(alphS)*cos(alphS)+
                         pow(cos(alphS),2)*pow(sin(alphD),2)*sigma3;
                    
               sigmazz = pow(sin(alphD),2)*(pow(sin(alphL),2)*sigma1+pow(cos(alphL),2)*sigma2)+pow(cos(alphD),2)*sigma3;

               sigmaxy = (pow(cos(alphL),2)*sigma1+pow(sin(alphL),2)*sigma2)*sin(alphS)*cos(alphS)-
                         (pow(sin(alphL),2)*sigma1+pow(cos(alphL),2)*sigma2)*pow(cos(alphD),2)*sin(alphS)*cos(alphS)+
                         (sigma1-sigma2)*sin(alphL)*cos(alphL)*cos(alphD)*(pow(cos(alphS),2)-pow(sin(alphS),2))-
                         pow(sin(alphD),2)*sin(alphS)*cos(alphS)*sigma3;

               sigmaxz = -(pow(sin(alphL),2)*sigma1+pow(cos(alphL),2)*sigma2)*sin(alphD)*cos(alphD)*sin(alphS)+
                         (sigma1-sigma2)*sin(alphL)*cos(alphL)*sin(alphD)*cos(alphS)+sin(alphD)*cos(alphD)*sin(alphS)*sigma3;

               sigmayz = (pow(sin(alphL),2)*sigma1+pow(cos(alphL),2)*sigma2)*sin(alphD)*cos(alphD)*cos(alphS)+
                         (sigma1-sigma2)*sin(alphL)*cos(alphL)*sin(alphD)*sin(alphS)-sin(alphD)*cos(alphD)*cos(alphS)*sigma3;

               sigmayx = sigmaxy;

               sigmazx = sigmaxz;

               sigmazy = sigmayz;
          }

          if(indexm==-1)
          loc_mu = mu0*(1.0+phy_mu[elem_pythsical[index_elem]]);
          if(indexm!=-1)
          {
               mag1 = anisom.value[3*(indexm-1)+1];
               mag2 = anisom.value[3*(indexm-1)+2];//主轴磁化率
               mag3 = anisom.value[3*(indexm-1)+3];

               alphmS = anisom.angle[3*(indexm-1)+1]*pi/180.0; 
               alphmD = anisom.angle[3*(indexm-1)+2]*pi/180.0;//欧拉旋转角(弧度)
               alphmL = anisom.angle[3*(indexm-1)+3]*pi/180.0;

               magxx = (pow(cos(alphmL),2)*mag1+pow(sin(alphmL),2)*mag2)*pow(cos(alphmS),2)+
                    (pow(sin(alphmL),2)*mag1+pow(cos(alphmL),2)*mag2)*pow(sin(alphmS),2)*pow(cos(alphmD),2)-
                    2.0*(mag1-mag2)*sin(alphmL)*cos(alphmL)*cos(alphmD)*sin(alphmS)*cos(alphmS)+
                    pow(sin(alphmS),2)*pow(sin(alphmD),2)*mag3;

               magyy = (pow(cos(alphmL),2)*mag1+pow(sin(alphmL),2)*mag2)*pow(sin(alphmS),2)+
                    (pow(sin(alphmL),2)*mag1+pow(cos(alphmL),2)*mag2)*pow(cos(alphmS),2)*pow(cos(alphmD),2)+
                    2.0*(mag1-mag2)*sin(alphmL)*cos(alphmL)*cos(alphmD)*sin(alphmS)*cos(alphmS)+
                    pow(cos(alphmS),2)*pow(sin(alphmD),2)*mag3;
                    
               magzz = pow(sin(alphmD),2)*(pow(sin(alphmL),2)*mag1+pow(cos(alphmL),2)*mag2)+pow(cos(alphmD),2)*mag3;

               magxy = (pow(cos(alphmL),2)*mag1+pow(sin(alphmL),2)*mag2)*sin(alphmS)*cos(alphmS)-
                    (pow(sin(alphmL),2)*mag1+pow(cos(alphmL),2)*mag2)*pow(cos(alphmD),2)*sin(alphmS)*cos(alphmS)+
                    (mag1-mag2)*sin(alphmL)*cos(alphmL)*cos(alphmD)*(pow(cos(alphmS),2)-pow(sin(alphmS),2))-
                    pow(sin(alphmD),2)*sin(alphmS)*cos(alphmS)*mag3;

               magxz = -(pow(sin(alphmL),2)*mag1+pow(cos(alphmL),2)*mag2)*sin(alphmD)*cos(alphmD)*sin(alphmS)+
                    (mag1-mag2)*sin(alphmL)*cos(alphmL)*sin(alphmD)*cos(alphmS)+sin(alphmD)*cos(alphmD)*sin(alphmS)*mag3;

               magyz = (pow(sin(alphmL),2)*mag1+pow(cos(alphmL),2)*mag2)*sin(alphmD)*cos(alphmD)*cos(alphmS)+
                    (mag1-mag2)*sin(alphmL)*cos(alphmL)*sin(alphmD)*sin(alphmS)-sin(alphmD)*cos(alphmD)*cos(alphmS)*mag3;

               magyx = magxy;

               magzx = magxz;

               magzy = magyz;

               Matrixmu[0][0]=mu0*(1.0+magxx); Matrixmu[0][1]=mu0*magxy;       Matrixmu[0][2]=mu0*magxz;
               Matrixmu[1][0]=mu0*magyx;       Matrixmu[1][1]=mu0*(1.0+magyy); Matrixmu[1][2]=mu0*magyz;
               Matrixmu[2][0]=mu0*magzx;       Matrixmu[2][1]=mu0*magzy;       Matrixmu[2][2]=mu0*(1.0+magzz);

               if(inverseMatrix(Matrixmu,MatrixV)!=1)
               {
                    printf("磁导率矩阵不可逆\n");
                    exit(0);
               }

               vxx=MatrixV[0][0]; vxy=MatrixV[0][1]; vxz=MatrixV[0][2];
               vyx=MatrixV[1][0]; vyy=MatrixV[1][1]; vyz=MatrixV[1][2];
               vzx=MatrixV[2][0]; vzy=MatrixV[2][1]; vzz=MatrixV[2][2];
               
          }

          ind_1=elem_node1[index_elem];
          ind_2=elem_node2[index_elem];
          ind_3=elem_node3[index_elem];
          ind_4=elem_node4[index_elem];

          /*******node_1,2,3,4_x,y,z************/
          x1=node_x[ind_1];
          x2=node_x[ind_2];
          x3=node_x[ind_3];
          x4=node_x[ind_4];

          y1=node_y[ind_1];
          y2=node_y[ind_2];
          y3=node_y[ind_3];
          y4=node_y[ind_4];

          z1=node_z[ind_1];
          z2=node_z[ind_2];
          z3=node_z[ind_3];
          z4=node_z[ind_4];

          Ve= (x2*y3*z4 - x2*y4*z3 - x3*y2*z4 + x3*y4*z2 + x4*y2*z3 - x4*y3*z2\
            - x1*y3*z4 + x1*y4*z3 + x3*y1*z4 - x3*y4*z1 - x4*y1*z3 + x4*y3*z1\
            + x1*y2*z4 - x1*y4*z2 - x2*y1*z4 + x2*y4*z1 + x4*y1*z2 - x4*y2*z1\
            - x1*y2*z3 + x1*y3*z2 + x2*y1*z3 - x2*y3*z1 - x3*y1*z2 + x3*y2*z1)/6.0;


          //a1 = x2*y3*z4 - x2*y4*z3 - x3*y2*z4 + x3*y4*z2 + x4*y2*z3 - x4*y3*z2;
          //a2 =-x1*y3*z4 + x1*y4*z3 + x3*y1*z4 - x3*y4*z1 - x4*y1*z3 + x4*y3*z1;
          //a3 = x1*y2*z4 - x1*y4*z2 - x2*y1*z4 + x2*y4*z1 + x4*y1*z2 - x4*y2*z1;
          //a4 =-x1*y2*z3 + x1*y3*z2 + x2*y1*z3 - x2*y3*z1 - x3*y1*z2 + x3*y2*z1;
          
          b1 =-y3*z4 + y4*z3 + y2*z4 - y4*z2 - y2*z3 + y3*z2;
          b2 = y3*z4 - y4*z3 - y1*z4 + y4*z1 + y1*z3 - y3*z1;
          b3 =-y2*z4 + y4*z2 + y1*z4 - y4*z1 - y1*z2 + y2*z1;
          b4 = y2*z3 - y3*z2 - y1*z3 + y3*z1 + y1*z2 - y2*z1;

          c1 =-x2*z4 + x2*z3 + x3*z4 - x3*z2 - x4*z3 + x4*z2;
          c2 = x1*z4 - x1*z3 - x3*z4 + x3*z1 + x4*z3 - x4*z1;
          c3 =-x1*z4 + x1*z2 + x2*z4 - x2*z1 - x4*z2 + x4*z1;
          c4 = x1*z3 - x1*z2 - x2*z3 + x2*z1 + x3*z2 - x3*z1;

          d1 =-x2*y3 + x2*y4 + x3*y2 - x3*y4 - x4*y2 + x4*y3;
          d2 = x1*y3 - x1*y4 - x3*y1 + x3*y4 + x4*y1 - x4*y3;
          d3 =-x1*y2 + x1*y4 + x2*y1 - x2*y4 - x4*y1 + x4*y2;
          d4 = x1*y2 - x1*y3 - x2*y1 + x2*y3 + x3*y1 - x3*y2;

          l1 = sqrt(pow(x1-x2,2)+pow(y1-y2,2)+pow(z1-z2,2));
          l2 = sqrt(pow(x1-x3,2)+pow(y1-y3,2)+pow(z1-z3,2));
          l3 = sqrt(pow(x1-x4,2)+pow(y1-y4,2)+pow(z1-z4,2));
          l4 = sqrt(pow(x2-x3,2)+pow(y2-y3,2)+pow(z2-z3,2));
          l5 = sqrt(pow(x2-x4,2)+pow(y2-y4,2)+pow(z2-z4,2));
          l6 = sqrt(pow(x3-x4,2)+pow(y3-y4,2)+pow(z3-z4,2));

          if(indexc==-1)
          {
               f11= b1*b1+c1*c1+d1*d1;
               f12= b1*b2+c1*c2+d1*d2;
               f13= b1*b3+c1*c3+d1*d3;
               f14= b1*b4+c1*c4+d1*d4;
               f22= b2*b2+c2*c2+d2*d2;
               f23= b2*b3+c2*c3+d2*d3;
               f24= b2*b4+c2*c4+d2*d4;
               f33= b3*b3+c3*c3+d3*d3;
               f34= b3*b4+c3*c4+d3*d4;
               f44= b4*b4+c4*c4+d4*d4;
          }
          else
          {
               f11= b1*b1*sigmaxx+c1*c1*sigmayy+d1*d1*sigmazz+b1*c1*sigmaxy+c1*b1*sigmayx+
                    b1*d1*sigmaxz+d1*b1*sigmazx+c1*d1*sigmayz+d1*c1*sigmazy;
               f12= b1*b2*sigmaxx+c1*c2*sigmayy+d1*d2*sigmazz+b1*c2*sigmaxy+c1*b2*sigmayx+
                    b1*d2*sigmaxz+d1*b2*sigmazx+c1*d2*sigmayz+d1*c2*sigmazy;
               f13= b1*b3*sigmaxx+c1*c3*sigmayy+d1*d3*sigmazz+b1*c3*sigmaxy+c1*b3*sigmayx+
                    b1*d3*sigmaxz+d1*b3*sigmazx+c1*d3*sigmayz+d1*c3*sigmazy;
               f14= b1*b4*sigmaxx+c1*c4*sigmayy+d1*d4*sigmazz+b1*c4*sigmaxy+c1*b4*sigmayx+
                    b1*d4*sigmaxz+d1*b4*sigmazx+c1*d4*sigmayz+d1*c4*sigmazy;
               f22= b2*b2*sigmaxx+c2*c2*sigmayy+d2*d2*sigmazz+b2*c2*sigmaxy+c2*b2*sigmayx+
                    b2*d2*sigmaxz+d2*b2*sigmazx+c2*d2*sigmayz+d2*c2*sigmazy;
               f23= b2*b3*sigmaxx+c2*c3*sigmayy+d2*d3*sigmazz+b2*c3*sigmaxy+c2*b3*sigmayx+
                    b2*d3*sigmaxz+d2*b3*sigmazx+c2*d3*sigmayz+d2*c3*sigmazy;
               f24= b2*b4*sigmaxx+c2*c4*sigmayy+d2*d4*sigmazz+b2*c4*sigmaxy+c2*b4*sigmayx+
                    b2*d4*sigmaxz+d2*b4*sigmazx+c2*d4*sigmayz+d2*c4*sigmazy;
               f33= b3*b3*sigmaxx+c3*c3*sigmayy+d3*d3*sigmazz+b3*c3*sigmaxy+c3*b3*sigmayx+
                    b3*d3*sigmaxz+d3*b3*sigmazx+c3*d3*sigmayz+d3*c3*sigmazy;
               f34= b3*b4*sigmaxx+c3*c4*sigmayy+d3*d4*sigmazz+b3*c4*sigmaxy+c3*b4*sigmayx+
                    b3*d4*sigmaxz+d3*b4*sigmazx+c3*d4*sigmayz+d3*c4*sigmazy;
               f44= b4*b4*sigmaxx+c4*c4*sigmayy+d4*d4*sigmazz+b4*c4*sigmaxy+c4*b4*sigmayx+
                    b4*d4*sigmaxz+d4*b4*sigmazx+c4*d4*sigmayz+d4*c4*sigmazy;
          }

          E_r=0.0;
          E_i=0.0;

          switch(index_item)
          {
               //------------------------------------------------------------------v
               case  0: //E11,F11
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d2-c2*d1)*(c1*d2-c2*d1)*vxx+(b2*d1-b1*d2)*(c1*d2-c2*d1)*vyx+(b1*c2-b2*c1)*(c1*d2-c2*d1)*vzx+
                                      (c1*d2-c2*d1)*(b2*d1-b1*d2)*vxy+(b2*d1-b1*d2)*(b2*d1-b1*d2)*vyy+(b1*c2-b2*c1)*(b2*d1-b1*d2)*vzy+
                                      (c1*d2-c2*d1)*(b1*c2-b2*c1)*vxz+(b2*d1-b1*d2)*(b1*c2-b2*c1)*vyz+(b1*c2-b2*c1)*(b1*c2-b2*c1)*vzz)*l1*l1*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d2-d1*c2)*(c1*d2-d1*c2)+(d1*b2-b1*d2)*(d1*b2-b1*d2)\
                           +(b1*c2-c1*b2)*(b1*c2-c1*b2))*l1*l1*Ve/pow(6*Ve,4);
                      E_i= l1*l1*(f22-f12+f11)/360/Ve;
                      break;
               case  1: //E12,F12
                      if(anisom.AnisoNumber>0)                               
                      E_r= ((c1*d2-c2*d1)*(c1*d3-c3*d1)*vxx+(b2*d1-b1*d2)*(c1*d3-c3*d1)*vyx+(b1*c2-b2*c1)*(c1*d3-c3*d1)*vzx+
                                      (c1*d2-c2*d1)*(b3*d1-b1*d3)*vxy+(b2*d1-b1*d2)*(b3*d1-b1*d3)*vyy+(b1*c2-b2*c1)*(b3*d1-b1*d3)*vzy+
                                      (c1*d2-c2*d1)*(b1*c3-b3*c1)*vxz+(b2*d1-b1*d2)*(b1*c3-b3*c1)*vyz+(b1*c2-b2*c1)*(b1*c3-b3*c1)*vzz)*l1*l2*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d2-d1*c2)*(c1*d3-d1*c3)+(d1*b2-b1*d2)*(d1*b3-b1*d3)\
                           +(b1*c2-c1*b2)*(b1*c3-c1*b3))*l1*l2*Ve/pow(6*Ve,4);
                      E_i= l1*l2*(2*f23-f12-f13+f11)/720/Ve;
                      if (ind_1>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case  2: //E13,F13    
                      if(anisom.AnisoNumber>0)                      
                      E_r= ((c1*d2-c2*d1)*(c1*d4-c4*d1)*vxx+(b2*d1-b1*d2)*(c1*d4-c4*d1)*vyx+(b1*c2-b2*c1)*(c1*d4-c4*d1)*vzx+
                                   (c1*d2-c2*d1)*(b4*d1-b1*d4)*vxy+(b2*d1-b1*d2)*(b4*d1-b1*d4)*vyy+(b1*c2-b2*c1)*(b4*d1-b1*d4)*vzy+
                                   (c1*d2-c2*d1)*(b1*c4-b4*c1)*vxz+(b2*d1-b1*d2)*(b1*c4-b4*c1)*vyz+(b1*c2-b2*c1)*(b1*c4-b4*c1)*vzz)*l1*l3*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d2-d1*c2)*(c1*d4-d1*c4)+(d1*b2-b1*d2)*(d1*b4-b1*d4)\
                           +(b1*c2-c1*b2)*(b1*c4-c1*b4))*l1*l3*Ve/pow(6*Ve,4);
                      E_i= l1*l3*(2*f24-f12-f14+f11)/720/Ve;
                      if (ind_1>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case  3: //E14,F14
                      if(anisom.AnisoNumber>0) 
                      E_r= ((c1*d2-c2*d1)*(c2*d3-c3*d2)*vxx+(b2*d1-b1*d2)*(c2*d3-c3*d2)*vyx+(b1*c2-b2*c1)*(c2*d3-c3*d2)*vzx+
                                      (c1*d2-c2*d1)*(b3*d2-b2*d3)*vxy+(b2*d1-b1*d2)*(b3*d2-b2*d3)*vyy+(b1*c2-b2*c1)*(b3*d2-b2*d3)*vzy+
                                      (c1*d2-c2*d1)*(b2*c3-b3*c2)*vxz+(b2*d1-b1*d2)*(b2*c3-b3*c2)*vyz+(b1*c2-b2*c1)*(b2*c3-b3*c2)*vzz)*l1*l4*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d2-d1*c2)*(c2*d3-d2*c3)+(d1*b2-b1*d2)*(d2*b3-b2*d3)\
                           +(b1*c2-c1*b2)*(b2*c3-c2*b3))*l1*l4*Ve/pow(6*Ve,4);
                      E_i= l1*l4*(f23-f22-2*f13+f12)/720/Ve;
                      if (ind_1>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_2>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case  4: //E15,F15
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d2-c2*d1)*(c4*d2-c2*d4)*vxx+(b2*d1-b1*d2)*(c4*d2-c2*d4)*vyx+(b1*c2-b2*c1)*(c4*d2-c2*d4)*vzx+
                                      (c1*d2-c2*d1)*(b2*d4-b4*d2)*vxy+(b2*d1-b1*d2)*(b2*d4-b4*d2)*vyy+(b1*c2-b2*c1)*(b2*d4-b4*d2)*vzy+
                                      (c1*d2-c2*d1)*(b4*c2-b2*c4)*vxz+(b2*d1-b1*d2)*(b4*c2-b2*c4)*vyz+(b1*c2-b2*c1)*(b4*c2-b2*c4)*vzz)*l1*l5*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d2-d1*c2)*(c4*d2-d4*c2)+(d1*b2-b1*d2)*(d4*b2-b4*d2)\
                           +(b1*c2-c1*b2)*(b4*c2-c4*b2))*l1*l5*Ve/pow(6*Ve,4);
                      E_i= l1*l5*(f22-f24-f12+2*f14)/720/Ve;
                      if (ind_1>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_4>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case  5: //E16,F16
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d2-c2*d1)*(c3*d4-c4*d3)*vxx+(b2*d1-b1*d2)*(c3*d4-c4*d3)*vyx+(b1*c2-b2*c1)*(c3*d4-c4*d3)*vzx+
                                      (c1*d2-c2*d1)*(b4*d3-b3*d4)*vxy+(b2*d1-b1*d2)*(b4*d3-b3*d4)*vyy+(b1*c2-b2*c1)*(b4*d3-b3*d4)*vzy+
                                      (c1*d2-c2*d1)*(b3*c4-b4*c3)*vxz+(b2*d1-b1*d2)*(b3*c4-b4*c3)*vyz+(b1*c2-b2*c1)*(b3*c4-b4*c3)*vzz)*l1*l6*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d2-d1*c2)*(c3*d4-d3*c4)+(d1*b2-b1*d2)*(d3*b4-b3*d4)\
                           +(b1*c2-c1*b2)*(b3*c4-c3*b4))*l1*l6*Ve/pow(6*Ve,4);
                      E_i= l1*l6*(f24-f23-f14+f13)/720/Ve;
                      if (ind_1>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_3>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               //------------------------------------------------------------------v
               case  6: //E21  
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d3-c3*d1)*(c1*d2-c2*d1)*vxx+(b3*d1-b1*d3)*(c1*d2-c2*d1)*vyx+(b1*c3-b3*c1)*(c1*d2-c2*d1)*vzx+
                                      (c1*d3-c3*d1)*(b2*d1-b1*d2)*vxy+(b3*d1-b1*d3)*(b2*d1-b1*d2)*vyy+(b1*c3-b3*c1)*(b2*d1-b1*d2)*vzy+
                                      (c1*d3-c3*d1)*(b1*c2-b2*c1)*vxz+(b3*d1-b1*d3)*(b1*c2-b2*c1)*vyz+(b1*c3-b3*c1)*(b1*c2-b2*c1)*vzz)*l2*l1*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d3-d1*c3)*(c1*d2-d1*c2)+(d1*b3-b1*d3)*(d1*b2-b1*d2)\
                           +(b1*c3-c1*b3)*(b1*c2-c1*b2))*l2*l1*Ve/pow(6*Ve,4);
                      E_i= l1*l2*(2*f23-f12-f13+f11)/720/Ve;
                      if (ind_1>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case  7: //E22,F22
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d3-c3*d1)*(c1*d3-c3*d1)*vxx+(b3*d1-b1*d3)*(c1*d3-c3*d1)*vyx+(b1*c3-b3*c1)*(c1*d3-c3*d1)*vzx+
                                      (c1*d3-c3*d1)*(b3*d1-b1*d3)*vxy+(b3*d1-b1*d3)*(b3*d1-b1*d3)*vyy+(b1*c3-b3*c1)*(b3*d1-b1*d3)*vzy+
                                      (c1*d3-c3*d1)*(b1*c3-b3*c1)*vxz+(b3*d1-b1*d3)*(b1*c3-b3*c1)*vyz+(b1*c3-b3*c1)*(b1*c3-b3*c1)*vzz)*l2*l2*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d3-d1*c3)*(c1*d3-d1*c3)+(d1*b3-b1*d3)*(d1*b3-b1*d3)\
                           +(b1*c3-c1*b3)*(b1*c3-c1*b3))*l2*l2*Ve/pow(6*Ve,4);
                      E_i= l2*l2*(f33-f13+f11)/360/Ve;
                      break;
               case  8: //E23,F23
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d3-c3*d1)*(c1*d4-c4*d1)*vxx+(b3*d1-b1*d3)*(c1*d4-c4*d1)*vyx+(b1*c3-b3*c1)*(c1*d4-c4*d1)*vzx+
                                      (c1*d3-c3*d1)*(b4*d1-b1*d4)*vxy+(b3*d1-b1*d3)*(b4*d1-b1*d4)*vyy+(b1*c3-b3*c1)*(b4*d1-b1*d4)*vzy+
                                      (c1*d3-c3*d1)*(b1*c4-b4*c1)*vxz+(b3*d1-b1*d3)*(b1*c4-b4*c1)*vyz+(b1*c3-b3*c1)*(b1*c4-b4*c1)*vzz)*l2*l3*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d3-d1*c3)*(c1*d4-d1*c4)+(d1*b3-b1*d3)*(d1*b4-b1*d4)\
                           +(b1*c3-c1*b3)*(b1*c4-c1*b4))*l2*l3*Ve/pow(6*Ve,4);
                      E_i= l2*l3*(2*f34-f13-f14+f11)/720/Ve;
                      if (ind_1>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case  9: //E24,F24
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d3-c3*d1)*(c2*d3-c3*d2)*vxx+(b3*d1-b1*d3)*(c2*d3-c3*d2)*vyx+(b1*c3-b3*c1)*(c2*d3-c3*d2)*vzx+
                                      (c1*d3-c3*d1)*(b3*d2-b2*d3)*vxy+(b3*d1-b1*d3)*(b3*d2-b2*d3)*vyy+(b1*c3-b3*c1)*(b3*d2-b2*d3)*vzy+
                                      (c1*d3-c3*d1)*(b2*c3-b3*c2)*vxz+(b3*d1-b1*d3)*(b2*c3-b3*c2)*vyz+(b1*c3-b3*c1)*(b2*c3-b3*c2)*vzz)*l2*l4*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d3-d1*c3)*(c2*d3-d2*c3)+(d1*b3-b1*d3)*(d2*b3-b2*d3)\
                           +(b1*c3-c1*b3)*(b2*c3-c2*b3))*l2*l4*Ve/pow(6*Ve,4);
                      E_i= l2*l4*(f33-f23-f13+2*f12)/720/Ve;
                      if (ind_1>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_2>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 10: //E25,F25
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d3-c3*d1)*(c4*d2-c2*d4)*vxx+(b3*d1-b1*d3)*(c4*d2-c2*d4)*vyx+(b1*c3-b3*c1)*(c4*d2-c2*d4)*vzx+
                                      (c1*d3-c3*d1)*(b2*d4-b4*d2)*vxy+(b3*d1-b1*d3)*(b2*d4-b4*d2)*vyy+(b1*c3-b3*c1)*(b2*d4-b4*d2)*vzy+
                                      (c1*d3-c3*d1)*(b4*c2-b2*c4)*vxz+(b3*d1-b1*d3)*(b4*c2-b2*c4)*vyz+(b1*c3-b3*c1)*(b4*c2-b2*c4)*vzz)*l2*l5*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d3-d1*c3)*(c4*d2-d4*c2)+(d1*b3-b1*d3)*(d4*b2-b4*d2)\
                           +(b1*c3-c1*b3)*(b4*c2-c4*b2))*l2*l5*Ve/pow(6*Ve,4);
                      E_i= l2*l5*(f23-f34-f12+f14)/720/Ve;
                      if (ind_1>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_4>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 11: //E26,F26
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d3-c3*d1)*(c3*d4-c4*d3)*vxx+(b3*d1-b1*d3)*(c3*d4-c4*d3)*vyx+(b1*c3-b3*c1)*(c3*d4-c4*d3)*vzx+
                                      (c1*d3-c3*d1)*(b4*d3-b3*d4)*vxy+(b3*d1-b1*d3)*(b4*d3-b3*d4)*vyy+(b1*c3-b3*c1)*(b4*d3-b3*d4)*vzy+
                                      (c1*d3-c3*d1)*(b3*c4-b4*c3)*vxz+(b3*d1-b1*d3)*(b3*c4-b4*c3)*vyz+(b1*c3-b3*c1)*(b3*c4-b4*c3)*vzz)*l2*l6*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d3-d1*c3)*(c3*d4-d3*c4)+(d1*b3-b1*d3)*(d3*b4-b3*d4)\
                           +(b1*c3-c1*b3)*(b3*c4-c3*b4))*l2*l6*Ve/pow(6*Ve,4);
                      E_i= l2*l6*(f13-f33-2*f14+f34)/720/Ve;
                      if (ind_1>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_3>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               //------------------------------------------------------------------v
               case 12: //E31  
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d4-c4*d1)*(c1*d2-c2*d1)*vxx+(b4*d1-b1*d4)*(c1*d2-c2*d1)*vyx+(b1*c4-b4*c1)*(c1*d2-c2*d1)*vzx+
                                      (c1*d4-c4*d1)*(b2*d1-b1*d2)*vxy+(b4*d1-b1*d4)*(b2*d1-b1*d2)*vyy+(b1*c4-b4*c1)*(b2*d1-b1*d2)*vzy+
                                      (c1*d4-c4*d1)*(b1*c2-b2*c1)*vxz+(b4*d1-b1*d4)*(b1*c2-b2*c1)*vyz+(b1*c4-b4*c1)*(b1*c2-b2*c1)*vzz)*l3*l1*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d4-d1*c4)*(c1*d2-d1*c2)+(d1*b4-b1*d4)*(d1*b2-b1*d2)\
                           +(b1*c4-c1*b4)*(b1*c2-c1*b2))*l3*l1*Ve/pow(6*Ve,4);
                      E_i= l1*l3*(2*f24-f12-f14+f11)/720/Ve;
                      if (ind_1>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 13: //E32 
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d4-c4*d1)*(c1*d3-c3*d1)*vxx+(b4*d1-b1*d4)*(c1*d3-c3*d1)*vyx+(b1*c4-b4*c1)*(c1*d3-c3*d1)*vzx+
                                      (c1*d4-c4*d1)*(b3*d1-b1*d3)*vxy+(b4*d1-b1*d4)*(b3*d1-b1*d3)*vyy+(b1*c4-b4*c1)*(b3*d1-b1*d3)*vzy+
                                      (c1*d4-c4*d1)*(b1*c3-b3*c1)*vxz+(b4*d1-b1*d4)*(b1*c3-b3*c1)*vyz+(b1*c4-b4*c1)*(b1*c3-b3*c1)*vzz)*l3*l2*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d4-d1*c4)*(c1*d3-d1*c3)+(d1*b4-b1*d4)*(d1*b3-b1*d3)\
                           +(b1*c4-c1*b4)*(b1*c3-c1*b3))*l3*l2*Ve/pow(6*Ve,4);
                      E_i= l2*l3*(2*f34-f13-f14+f11)/720/Ve;
                      if (ind_1>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 14: //E33 F33 
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d4-c4*d1)*(c1*d4-c4*d1)*vxx+(b4*d1-b1*d4)*(c1*d4-c4*d1)*vyx+(b1*c4-b4*c1)*(c1*d4-c4*d1)*vzx+
                                      (c1*d4-c4*d1)*(b4*d1-b1*d4)*vxy+(b4*d1-b1*d4)*(b4*d1-b1*d4)*vyy+(b1*c4-b4*c1)*(b4*d1-b1*d4)*vzy+
                                      (c1*d4-c4*d1)*(b1*c4-b4*c1)*vxz+(b4*d1-b1*d4)*(b1*c4-b4*c1)*vyz+(b1*c4-b4*c1)*(b1*c4-b4*c1)*vzz)*l3*l3*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d4-d1*c4)*(c1*d4-d1*c4)+(d1*b4-b1*d4)*(d1*b4-b1*d4)\
                           +(b1*c4-c1*b4)*(b1*c4-c1*b4))*l3*l3*Ve/pow(6*Ve,4);
                      E_i= l3*l3*(f44-f14+f11)/360/Ve;
                      break;
               case 15: //E34 F34
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d4-c4*d1)*(c2*d3-c3*d2)*vxx+(b4*d1-b1*d4)*(c2*d3-c3*d2)*vyx+(b1*c4-b4*c1)*(c2*d3-c3*d2)*vzx+
                                      (c1*d4-c4*d1)*(b3*d2-b2*d3)*vxy+(b4*d1-b1*d4)*(b3*d2-b2*d3)*vyy+(b1*c4-b4*c1)*(b3*d2-b2*d3)*vzy+
                                      (c1*d4-c4*d1)*(b2*c3-b3*c2)*vxz+(b4*d1-b1*d4)*(b2*c3-b3*c2)*vyz+(b1*c4-b4*c1)*(b2*c3-b3*c2)*vzz)*l3*l4*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d4-d1*c4)*(c2*d3-d2*c3)+(d1*b4-b1*d4)*(d2*b3-b2*d3)\
                           +(b1*c4-c1*b4)*(b2*c3-c2*b3))*l3*l4*Ve/pow(6*Ve,4);
                      E_i= l3*l4*(f34-f24-f13+f12)/720/Ve;
                      if (ind_1>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_2>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 16: //E35 F35 
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d4-c4*d1)*(c4*d2-c2*d4)*vxx+(b4*d1-b1*d4)*(c4*d2-c2*d4)*vyx+(b1*c4-b4*c1)*(c4*d2-c2*d4)*vzx+
                                      (c1*d4-c4*d1)*(b2*d4-b4*d2)*vxy+(b4*d1-b1*d4)*(b2*d4-b4*d2)*vyy+(b1*c4-b4*c1)*(b2*d4-b4*d2)*vzy+
                                      (c1*d4-c4*d1)*(b4*c2-b2*c4)*vxz+(b4*d1-b1*d4)*(b4*c2-b2*c4)*vyz+(b1*c4-b4*c1)*(b4*c2-b2*c4)*vzz)*l3*l5*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d4-d1*c4)*(c4*d2-d4*c2)+(d1*b4-b1*d4)*(d4*b2-b4*d2)\
                           +(b1*c4-c1*b4)*(b4*c2-c4*b2))*l3*l5*Ve/pow(6*Ve,4);
                      E_i= l3*l5*(f24-f44-2*f12+f14)/720/Ve;
                      if (ind_1>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_4>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 17: //E36 F36
                      if(anisom.AnisoNumber>0)
                      E_r= ((c1*d4-c4*d1)*(c3*d4-c4*d3)*vxx+(b4*d1-b1*d4)*(c3*d4-c4*d3)*vyx+(b1*c4-b4*c1)*(c3*d4-c4*d3)*vzx+
                                      (c1*d4-c4*d1)*(b4*d3-b3*d4)*vxy+(b4*d1-b1*d4)*(b4*d3-b3*d4)*vyy+(b1*c4-b4*c1)*(b4*d3-b3*d4)*vzy+
                                      (c1*d4-c4*d1)*(b3*c4-b4*c3)*vxz+(b4*d1-b1*d4)*(b3*c4-b4*c3)*vyz+(b1*c4-b4*c1)*(b3*c4-b4*c3)*vzz)*l3*l6*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c1*d4-d1*c4)*(c3*d4-d3*c4)+(d1*b4-b1*d4)*(d3*b4-b3*d4)\
                           +(b1*c4-c1*b4)*(b3*c4-c3*b4))*l3*l6*Ve/pow(6*Ve,4);
                      E_i= l3*l6*(f44-f34-f14+2*f13)/720/Ve;
                      if (ind_1>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_3>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               //------------------------------------------------------------------v
               case 18: //E41
                      if(anisom.AnisoNumber>0)
                      E_r= ((c2*d3-c3*d2)*(c1*d2-c2*d1)*vxx+(b3*d2-b2*d3)*(c1*d2-c2*d1)*vyx+(b2*c3-b3*c2)*(c1*d2-c2*d1)*vzx+
                                      (c2*d3-c3*d2)*(b2*d1-b1*d2)*vxy+(b3*d2-b2*d3)*(b2*d1-b1*d2)*vyy+(b2*c3-b3*c2)*(b2*d1-b1*d2)*vzy+
                                      (c2*d3-c3*d2)*(b1*c2-b2*c1)*vxz+(b3*d2-b2*d3)*(b1*c2-b2*c1)*vyz+(b2*c3-b3*c2)*(b1*c2-b2*c1)*vzz)*l4*l1*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c2*d3-d2*c3)*(c1*d2-d1*c2)+(d2*b3-b2*d3)*(d1*b2-b1*d2)\
                           +(b2*c3-c2*b3)*(b1*c2-c1*b2))*l4*l1*Ve/pow(6*Ve,4);
                      E_i= l1*l4*(f23-f22-2*f13+f12)/720/Ve;
                      if (ind_2>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 19: //E42    
                      if(anisom.AnisoNumber>0)                  
                      E_r= ((c2*d3-c3*d2)*(c1*d3-c3*d1)*vxx+(b3*d2-b2*d3)*(c1*d3-c3*d1)*vyx+(b2*c3-b3*c2)*(c1*d3-c3*d1)*vzx+
                                      (c2*d3-c3*d2)*(b3*d1-b1*d3)*vxy+(b3*d2-b2*d3)*(b3*d1-b1*d3)*vyy+(b2*c3-b3*c2)*(b3*d1-b1*d3)*vzy+
                                      (c2*d3-c3*d2)*(b1*c3-b3*c1)*vxz+(b3*d2-b2*d3)*(b1*c3-b3*c1)*vyz+(b2*c3-b3*c2)*(b1*c3-b3*c1)*vzz)*l4*l2*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c2*d3-d2*c3)*(c1*d3-d1*c3)+(d2*b3-b2*d3)*(d1*b3-b1*d3)\
                           +(b2*c3-c2*b3)*(b1*c3-c1*b3))*l4*l2*Ve/pow(6*Ve,4);
                      E_i= l2*l4*(f33-f23-f13+2*f12)/720/Ve;
                      if (ind_2>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 20: //E43  
                      if(anisom.AnisoNumber>0)                        
                      E_r= ((c2*d3-c3*d2)*(c1*d4-c4*d1)*vxx+(b3*d2-b2*d3)*(c1*d4-c4*d1)*vyx+(b2*c3-b3*c2)*(c1*d4-c4*d1)*vzx+
                                      (c2*d3-c3*d2)*(b4*d1-b1*d4)*vxy+(b3*d2-b2*d3)*(b4*d1-b1*d4)*vyy+(b2*c3-b3*c2)*(b4*d1-b1*d4)*vzy+
                                      (c2*d3-c3*d2)*(b1*c4-b4*c1)*vxz+(b3*d2-b2*d3)*(b1*c4-b4*c1)*vyz+(b2*c3-b3*c2)*(b1*c4-b4*c1)*vzz)*l4*l3*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c2*d3-d2*c3)*(c1*d4-d1*c4)+(d2*b3-b2*d3)*(d1*b4-b1*d4)\
                           +(b2*c3-c2*b3)*(b1*c4-c1*b4))*l4*l3*Ve/pow(6*Ve,4);
                      E_i= l3*l4*(f34-f24-f13+f12)/720/Ve;
                      if (ind_2>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 21: //E44 F44
                      if(anisom.AnisoNumber>0) 
                      E_r= ((c2*d3-c3*d2)*(c2*d3-c3*d2)*vxx+(b3*d2-b2*d3)*(c2*d3-c3*d2)*vyx+(b2*c3-b3*c2)*(c2*d3-c3*d2)*vzx+
                                      (c2*d3-c3*d2)*(b3*d2-b2*d3)*vxy+(b3*d2-b2*d3)*(b3*d2-b2*d3)*vyy+(b2*c3-b3*c2)*(b3*d2-b2*d3)*vzy+
                                      (c2*d3-c3*d2)*(b2*c3-b3*c2)*vxz+(b3*d2-b2*d3)*(b2*c3-b3*c2)*vyz+(b2*c3-b3*c2)*(b2*c3-b3*c2)*vzz)*l4*l4*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c2*d3-d2*c3)*(c2*d3-d2*c3)+(d2*b3-b2*d3)*(d2*b3-b2*d3)\
                           +(b2*c3-c2*b3)*(b2*c3-c2*b3))*l4*l4*Ve/pow(6*Ve,4);
                      E_i= l4*l4*(f33-f23+f22)/360/Ve;
                      break;
               case 22: //E45 F45
                      if(anisom.AnisoNumber>0) 
                      E_r= ((c2*d3-c3*d2)*(c4*d2-c2*d4)*vxx+(b3*d2-b2*d3)*(c4*d2-c2*d4)*vyx+(b2*c3-b3*c2)*(c4*d2-c2*d4)*vzx+
                                      (c2*d3-c3*d2)*(b2*d4-b4*d2)*vxy+(b3*d2-b2*d3)*(b2*d4-b4*d2)*vyy+(b2*c3-b3*c2)*(b2*d4-b4*d2)*vzy+
                                      (c2*d3-c3*d2)*(b4*c2-b2*c4)*vxz+(b3*d2-b2*d3)*(b4*c2-b2*c4)*vyz+(b2*c3-b3*c2)*(b4*c2-b2*c4)*vzz)*l4*l5*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c2*d3-d2*c3)*(c4*d2-d4*c2)+(d2*b3-b2*d3)*(d4*b2-b4*d2)\
                           +(b2*c3-c2*b3)*(b4*c2-c4*b2))*l4*l5*Ve/pow(6*Ve,4);
                      E_i= l4*l5*(f23-2*f34-f22+f24)/720/Ve;
                      if (ind_2>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_4>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 23: //E46 F46
                      if(anisom.AnisoNumber>0)
                      E_r= ((c2*d3-c3*d2)*(c3*d4-c4*d3)*vxx+(b3*d2-b2*d3)*(c3*d4-c4*d3)*vyx+(b2*c3-b3*c2)*(c3*d4-c4*d3)*vzx+
                                      (c2*d3-c3*d2)*(b4*d3-b3*d4)*vxy+(b3*d2-b2*d3)*(b4*d3-b3*d4)*vyy+(b2*c3-b3*c2)*(b4*d3-b3*d4)*vzy+
                                      (c2*d3-c3*d2)*(b3*c4-b4*c3)*vxz+(b3*d2-b2*d3)*(b3*c4-b4*c3)*vyz+(b2*c3-b3*c2)*(b3*c4-b4*c3)*vzz)*l4*l6*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c2*d3-d2*c3)*(c3*d4-d3*c4)+(d2*b3-b2*d3)*(d3*b4-b3*d4)\
                           +(b2*c3-c2*b3)*(b3*c4-c3*b4))*l4*l6*Ve/pow(6*Ve,4);
                      E_i= l4*l6*(f34-f33-2*f24+f23)/720/Ve;
                      if (ind_2>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_3>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;                         
               //------------------------------------------------------------------v
               case 24: //E51 
                      if(anisom.AnisoNumber>0)
                      E_r= ((c4*d2-c2*d4)*(c1*d2-c2*d1)*vxx+(b2*d4-b4*d2)*(c1*d2-c2*d1)*vyx+(b4*c2-b2*c4)*(c1*d2-c2*d1)*vzx+
                                      (c4*d2-c2*d4)*(b2*d1-b1*d2)*vxy+(b2*d4-b4*d2)*(b2*d1-b1*d2)*vyy+(b4*c2-b2*c4)*(b2*d1-b1*d2)*vzy+
                                      (c4*d2-c2*d4)*(b1*c2-b2*c1)*vxz+(b2*d4-b4*d2)*(b1*c2-b2*c1)*vyz+(b4*c2-b2*c4)*(b1*c2-b2*c1)*vzz)*l5*l1*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c4*d2-d4*c2)*(c1*d2-d1*c2)+(d4*b2-b4*d2)*(d1*b2-b1*d2)\
                           +(b4*c2-c4*b2)*(b1*c2-c1*b2))*l5*l1*Ve/pow(6*Ve,4);
                      E_i= l1*l5*(f22-f24-f12+2*f14)/720/Ve;
                      if (ind_4>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 25: //E52
                      if(anisom.AnisoNumber>0)
                      E_r= ((c4*d2-c2*d4)*(c1*d3-c3*d1)*vxx+(b2*d4-b4*d2)*(c1*d3-c3*d1)*vyx+(b4*c2-b2*c4)*(c1*d3-c3*d1)*vzx+
                                      (c4*d2-c2*d4)*(b3*d1-b1*d3)*vxy+(b2*d4-b4*d2)*(b3*d1-b1*d3)*vyy+(b4*c2-b2*c4)*(b3*d1-b1*d3)*vzy+
                                      (c4*d2-c2*d4)*(b1*c3-b3*c1)*vxz+(b2*d4-b4*d2)*(b1*c3-b3*c1)*vyz+(b4*c2-b2*c4)*(b1*c3-b3*c1)*vzz)*l5*l2*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c4*d2-d4*c2)*(c1*d3-d1*c3)+(d4*b2-b4*d2)*(d1*b3-b1*d3)\
                           +(b4*c2-c4*b2)*(b1*c3-c1*b3))*l5*l2*Ve/pow(6*Ve,4);
                      E_i= l2*l5*(f23-f34-f12+f14)/720/Ve;
                      if (ind_4>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 26: //E53
                      if(anisom.AnisoNumber>0)
                      E_r= ((c4*d2-c2*d4)*(c1*d4-c4*d1)*vxx+(b2*d4-b4*d2)*(c1*d4-c4*d1)*vyx+(b4*c2-b2*c4)*(c1*d4-c4*d1)*vzx+
                                      (c4*d2-c2*d4)*(b4*d1-b1*d4)*vxy+(b2*d4-b4*d2)*(b4*d1-b1*d4)*vyy+(b4*c2-b2*c4)*(b4*d1-b1*d4)*vzy+
                                      (c4*d2-c2*d4)*(b1*c4-b4*c1)*vxz+(b2*d4-b4*d2)*(b1*c4-b4*c1)*vyz+(b4*c2-b2*c4)*(b1*c4-b4*c1)*vzz)*l5*l3*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c4*d2-d4*c2)*(c1*d4-d1*c4)+(d4*b2-b4*d2)*(d1*b4-b1*d4)\
                           +(b4*c2-c4*b2)*(b1*c4-c1*b4))*l5*l3*Ve/pow(6*Ve,4);
                      E_i= l3*l5*(f24-f44-2*f12+f14)/720/Ve;
                      if (ind_4>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 27: //E54
                      if(anisom.AnisoNumber>0)
                      E_r= ((c4*d2-c2*d4)*(c2*d3-c3*d2)*vxx+(b2*d4-b4*d2)*(c2*d3-c3*d2)*vyx+(b4*c2-b2*c4)*(c2*d3-c3*d2)*vzx+
                                      (c4*d2-c2*d4)*(b3*d2-b2*d3)*vxy+(b2*d4-b4*d2)*(b3*d2-b2*d3)*vyy+(b4*c2-b2*c4)*(b3*d2-b2*d3)*vzy+
                                      (c4*d2-c2*d4)*(b2*c3-b3*c2)*vxz+(b2*d4-b4*d2)*(b2*c3-b3*c2)*vyz+(b4*c2-b2*c4)*(b2*c3-b3*c2)*vzz)*l5*l4*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c4*d2-d4*c2)*(c2*d3-d2*c3)+(d4*b2-b4*d2)*(d2*b3-b2*d3)\
                           +(b4*c2-c4*b2)*(b2*c3-c2*b3))*l5*l4*Ve/pow(6*Ve,4);
                      E_i= l4*l5*(f23-2*f34-f22+f24)/720/Ve;
                      if (ind_4>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_2>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 28: //E55 F55
                      if(anisom.AnisoNumber>0)
                      E_r= ((c4*d2-c2*d4)*(c4*d2-c2*d4)*vxx+(b2*d4-b4*d2)*(c4*d2-c2*d4)*vyx+(b4*c2-b2*c4)*(c4*d2-c2*d4)*vzx+
                                      (c4*d2-c2*d4)*(b2*d4-b4*d2)*vxy+(b2*d4-b4*d2)*(b2*d4-b4*d2)*vyy+(b4*c2-b2*c4)*(b2*d4-b4*d2)*vzy+
                                      (c4*d2-c2*d4)*(b4*c2-b2*c4)*vxz+(b2*d4-b4*d2)*(b4*c2-b2*c4)*vyz+(b4*c2-b2*c4)*(b4*c2-b2*c4)*vzz)*l5*l5*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c4*d2-d4*c2)*(c4*d2-d4*c2)+(d4*b2-b4*d2)*(d4*b2-b4*d2)\
                           +(b4*c2-c4*b2)*(b4*c2-c4*b2))*l5*l5*Ve/pow(6*Ve,4);
                      E_i= l5*l5*(f22-f24+f44)/360/Ve;
                      break;
               case 29: //E56 F56
                      if(anisom.AnisoNumber>0)
                      E_r= ((c4*d2-c2*d4)*(c3*d4-c4*d3)*vxx+(b2*d4-b4*d2)*(c3*d4-c4*d3)*vyx+(b4*c2-b2*c4)*(c3*d4-c4*d3)*vzx+
                                      (c4*d2-c2*d4)*(b4*d3-b3*d4)*vxy+(b2*d4-b4*d2)*(b4*d3-b3*d4)*vyy+(b4*c2-b2*c4)*(b4*d3-b3*d4)*vzy+
                                      (c4*d2-c2*d4)*(b3*c4-b4*c3)*vxz+(b2*d4-b4*d2)*(b3*c4-b4*c3)*vyz+(b4*c2-b2*c4)*(b3*c4-b4*c3)*vzz)*l5*l6*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c4*d2-d4*c2)*(c3*d4-d3*c4)+(d4*b2-b4*d2)*(d3*b4-b3*d4)\
                           +(b4*c2-c4*b2)*(b3*c4-c3*b4))*l5*l6*Ve/pow(6*Ve,4);
                      E_i= l5*l6*(f24-2*f23-f44+f34)/720/Ve;
                      if (ind_4>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_3>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;                         
               //------------------------------------------------------------------v
               case 30: //E61  
                      if(anisom.AnisoNumber>0)
                      E_r= ((c3*d4-c4*d3)*(c1*d2-c2*d1)*vxx+(b4*d3-b3*d4)*(c1*d2-c2*d1)*vyx+(b3*c4-b4*c3)*(c1*d2-c2*d1)*vzx+
                                      (c3*d4-c4*d3)*(b2*d1-b1*d2)*vxy+(b4*d3-b3*d4)*(b2*d1-b1*d2)*vyy+(b3*c4-b4*c3)*(b2*d1-b1*d2)*vzy+
                                      (c3*d4-c4*d3)*(b1*c2-b2*c1)*vxz+(b4*d3-b3*d4)*(b1*c2-b2*c1)*vyz+(b3*c4-b4*c3)*(b1*c2-b2*c1)*vzz)*l6*l1*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c3*d4-d3*c4)*(c1*d2-d1*c2)+(d3*b4-b3*d4)*(d1*b2-b1*d2)\
                           +(b3*c4-c3*b4)*(b1*c2-c1*b2))*l6*l1*Ve/pow(6*Ve,4);
                      E_i= l1*l6*(f24-f23-f14+f13)/720/Ve;
                      if (ind_3>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;                         
               case 31: //E62 
                      if(anisom.AnisoNumber>0)
                      E_r= ((c3*d4-c4*d3)*(c1*d3-c3*d1)*vxx+(b4*d3-b3*d4)*(c1*d3-c3*d1)*vyx+(b3*c4-b4*c3)*(c1*d3-c3*d1)*vzx+
                                      (c3*d4-c4*d3)*(b3*d1-b1*d3)*vxy+(b4*d3-b3*d4)*(b3*d1-b1*d3)*vyy+(b3*c4-b4*c3)*(b3*d1-b1*d3)*vzy+
                                      (c3*d4-c4*d3)*(b1*c3-b3*c1)*vxz+(b4*d3-b3*d4)*(b1*c3-b3*c1)*vyz+(b3*c4-b4*c3)*(b1*c3-b3*c1)*vzz)*l6*l2*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c3*d4-d3*c4)*(c1*d3-d1*c3)+(d3*b4-b3*d4)*(d1*b3-b1*d3)\
                           +(b3*c4-c3*b4)*(b1*c3-c1*b3))*l6*l2*Ve/pow(6*Ve,4);
                      E_i= l2*l6*(f13-f33-2*f14+f34)/720/Ve;
                      if (ind_3>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 32: //E63  
                      if(anisom.AnisoNumber>0)
                      E_r= ((c3*d4-c4*d3)*(c1*d4-c4*d1)*vxx+(b4*d3-b3*d4)*(c1*d4-c4*d1)*vyx+(b3*c4-b4*c3)*(c1*d4-c4*d1)*vzx+
                                      (c3*d4-c4*d3)*(b4*d1-b1*d4)*vxy+(b4*d3-b3*d4)*(b4*d1-b1*d4)*vyy+(b3*c4-b4*c3)*(b4*d1-b1*d4)*vzy+
                                      (c3*d4-c4*d3)*(b1*c4-b4*c1)*vxz+(b4*d3-b3*d4)*(b1*c4-b4*c1)*vyz+(b3*c4-b4*c3)*(b1*c4-b4*c1)*vzz)*l6*l3*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c3*d4-d3*c4)*(c1*d4-d1*c4)+(d3*b4-b3*d4)*(d1*b4-b1*d4)\
                           +(b3*c4-c3*b4)*(b1*c4-c1*b4))*l6*l3*Ve/pow(6*Ve,4);
                      E_i= l3*l6*(f44-f34-f14+2*f13)/720/Ve; 
                      if (ind_3>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_1>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 33: //E64  
                      if(anisom.AnisoNumber>0)
                      E_r= ((c3*d4-c4*d3)*(c2*d3-c3*d2)*vxx+(b4*d3-b3*d4)*(c2*d3-c3*d2)*vyx+(b3*c4-b4*c3)*(c2*d3-c3*d2)*vzx+
                                      (c3*d4-c4*d3)*(b3*d2-b2*d3)*vxy+(b4*d3-b3*d4)*(b3*d2-b2*d3)*vyy+(b3*c4-b4*c3)*(b3*d2-b2*d3)*vzy+
                                      (c3*d4-c4*d3)*(b2*c3-b3*c2)*vxz+(b4*d3-b3*d4)*(b2*c3-b3*c2)*vyz+(b3*c4-b4*c3)*(b2*c3-b3*c2)*vzz)*l6*l4*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c3*d4-d3*c4)*(c2*d3-d2*c3)+(d3*b4-b3*d4)*(d2*b3-b2*d3)\
                           +(b3*c4-c3*b4)*(b2*c3-c2*b3))*l6*l4*Ve/pow(6*Ve,4);
                      E_i= l4*l6*(f34-f33-2*f24+f23)/720/Ve;
                      if (ind_3>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_2>ind_3) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 34: //E65
                      if(anisom.AnisoNumber>0)  
                      E_r= ((c3*d4-c4*d3)*(c4*d2-c2*d4)*vxx+(b4*d3-b3*d4)*(c4*d2-c2*d4)*vyx+(b3*c4-b4*c3)*(c4*d2-c2*d4)*vzx+
                                      (c3*d4-c4*d3)*(b2*d4-b4*d2)*vxy+(b4*d3-b3*d4)*(b2*d4-b4*d2)*vyy+(b3*c4-b4*c3)*(b2*d4-b4*d2)*vzy+
                                      (c3*d4-c4*d3)*(b4*c2-b2*c4)*vxz+(b4*d3-b3*d4)*(b4*c2-b2*c4)*vyz+(b3*c4-b4*c3)*(b4*c2-b2*c4)*vzz)*l6*l5*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c3*d4-d3*c4)*(c4*d2-d4*c2)+(d3*b4-b3*d4)*(d4*b2-b4*d2)\
                           +(b3*c4-c3*b4)*(b4*c2-c4*b2))*l6*l5*Ve/pow(6*Ve,4);
                      E_i= l5*l6*(f24-2*f23-f44+f34)/720/Ve;
                      if (ind_3>ind_4) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      if (ind_4>ind_2) {E_r=E_r*(-1.0);E_i=E_i*(-1.0);}
                      break;
               case 35: //E66
                      if(anisom.AnisoNumber>0)   
                      E_r= ((c3*d4-c4*d3)*(c3*d4-c4*d3)*vxx+(b4*d3-b3*d4)*(c3*d4-c4*d3)*vyx+(b3*c4-b4*c3)*(c3*d4-c4*d3)*vzx+
                                      (c3*d4-c4*d3)*(b4*d3-b3*d4)*vxy+(b4*d3-b3*d4)*(b4*d3-b3*d4)*vyy+(b3*c4-b4*c3)*(b4*d3-b3*d4)*vzy+
                                      (c3*d4-c4*d3)*(b3*c4-b4*c3)*vxz+(b4*d3-b3*d4)*(b3*c4-b4*c3)*vyz+(b3*c4-b4*c3)*(b3*c4-b4*c3)*vzz)*l6*l6*Ve/pow(6*Ve,4);
                      else
                      E_r= ((c3*d4-d3*c4)*(c3*d4-d3*c4)+(d3*b4-b3*d4)*(d3*b4-b3*d4)\
                           +(b3*c4-c3*b4)*(b3*c4-c3*b4))*l6*l6*Ve/pow(6*Ve,4);
                      E_i= l6*l6*(f44-f34+f33)/360/Ve;
                      break;
          }//end_switch                   
      
          matrix_complex_real[i]=4.0*E_r;
          //matrix_complex_imag[i]=(-1.0)*E_i*cond*mu0;
          
          if(indexc==-1)
          {
               if(anisom.AnisoNumber>0)
               {
                    matrix_complex_imag[i]=(-1.0)*E_i*cond;
               }
               else
               {
                    matrix_complex_imag[i]=(-1.0)*E_i*cond*loc_mu;
               }
          }

          if(indexc!=-1)
          {
               if(anisom.AnisoNumber>0)
               {
                    matrix_complex_imag[i]=(-1.0)*E_i;
               }
               else
               {
                    matrix_complex_imag[i]=(-1.0)*E_i*loc_mu;
               }
          }

     }     
 
     //for (i=0;i<=length-1;i++) 
     //printf("i=%d,j=%d,r=%lf,i=%lf\n",matrix_i_loc[i],matrix_j_loc[i],matrix_complex_real[i],matrix_complex_imag[i]);

     //exit(0);
 
     if (myrank==0)
     {
          printf("-----------Elem_analyse_done------------\n"); 
     }

}
//-------------------------------------------------------------------------------------------
// Elem_assemble
//-------------------------------------------------------------------------------------------
void Elem_assemble()
{
     int i,j;
     int temp;

     temp=1;

     for (i=0;i<=item_num-2;i++)
     {

          if (matrix_i_loc[i]!=matrix_i_loc[i+1] || matrix_j_loc[i]!=matrix_j_loc[i+1])
          {
               temp++;
          }
     }

     matrix_number=temp;
     matrix_i=(int*) malloc( (matrix_number)*sizeof(int) );
     matrix_j=(int*) malloc( (matrix_number)*sizeof(int) );
     matrix_a_r  =(double*) malloc( (matrix_number)*sizeof(double) );
     matrix_a_i  =(double*) malloc( (matrix_number)*sizeof(double) );

     for (i=0;i<=matrix_number-1;i++)
          matrix_a_r[i]=0.0;
     for (i=0;i<=matrix_number-1;i++)
          matrix_a_i[i]=0.0;
     
     matrix_a_r[0]=matrix_complex_real[0];
     matrix_a_i[0]=matrix_complex_imag[0];

     matrix_i[0]  =matrix_i_loc[0];
     matrix_j[0]  =matrix_j_loc[0];

     j=0;
     for (i=0;i<=item_num-2;i++)
     {
         if (matrix_i_loc[i]!=matrix_i_loc[i+1] || matrix_j_loc[i]!=matrix_j_loc[i+1])
         {         
              j++;
              matrix_a_r[j]=matrix_a_r[j]+matrix_complex_real[i+1];
              matrix_a_i[j]=matrix_a_i[j]+matrix_complex_imag[i+1];
              matrix_i[j]  =matrix_i_loc[i+1];
              matrix_j[j]  =matrix_j_loc[i+1];
         }
         else 
         {
              matrix_a_r[j]=matrix_a_r[j]+matrix_complex_real[i+1];
              matrix_a_i[j]=matrix_a_i[j]+matrix_complex_imag[i+1];
         }        
     }
     
     /********************verification*******************/
     
     printf("myrank=%d,matrix_number=%d\n",myrank,matrix_number);
     MPI_Barrier(MPI_COMM_WORLD);

     /*if (myrank==0)
     {
          for (i=0;i<=matrix_number-1;i++)
               printf("i=%d,j=%d,r=%lf,i=%lf\n",matrix_i[i],matrix_j[i],matrix_a_r[i],matrix_a_i[i]);
     }
     MPI_Barrier(MPI_COMM_WORLD);
     sleep(1);
     */

     free(matrix_i_loc);
     free(matrix_j_loc);
     free(matrix_complex_real);
     free(matrix_complex_imag);

     if (myrank==0)
     {
          printf("-----------Elem_assemble_done-----------\n"); 
     }

}
//-------------------------------------------------------------------------------------------
// Process_divide
//-------------------------------------------------------------------------------------------
void Process_divide()
{
     /*******Process split***********/       
     Create_mpi_newcomm();
     
     /*******Matrix_merge************/ 
     Matrix_merge_FRE();

}
//-------------------------------------------------------------------------------------------
// Matrix_Deform
//-------------------------------------------------------------------------------------------
void Matrix_Deform()
{
     int    i,j,k,l;
     int    temp;
     int    temp_j;
 
     int    b_i_start;
     int    b_i_end;
     int    b_j_start;
     int    b_j_end;

     int   *matrix_i_rr;
     int   *matrix_j_rr;
     double*matrix_a_r_rr;
     double*matrix_a_i_rr;

     /*********************remove row **********************************/ 
     //find boundary judge begin and end;
     if (matrix_i[0] > bound_edge[bound_edge_total])
     {
          b_i_start=bound_edge_total;
          b_i_end  =bound_edge_total;
     }
     else if (matrix_i[matrix_number-1] < bound_edge[1])
     {
          b_i_start=0;
          b_i_end  =0; 
     }
     else
     {
          for (i=1;i<=bound_edge_total;i++)
          {
               if (matrix_i[0]<=bound_edge[i]) 
               {    
                    b_i_start=i;
                    break;
               }
          }

          for (i=1;i<=bound_edge_total;i++)
          {
               if (matrix_i[matrix_number-1]<=bound_edge[i])
               {    
                    b_i_end=i;
                    break;
               }
          }

          if (matrix_i[matrix_number-1]>=bound_edge[bound_edge_total])
               b_i_end=bound_edge_total;
     }
      
     temp=0;
     j=0;
     if (b_i_start==0 || b_i_start== bound_edge_total)
     {
         num_out_row=0;
     }
     else
     {
          for (i=b_i_start;i<=b_i_end;i++)
          {
               while (matrix_i[j] <bound_edge[i] )
               {
                    j++;
                    if (j==matrix_number) break;
               }

               if (j==matrix_number) break;

               while (matrix_i[j]==bound_edge[i] )
               {
                    j++;
                    temp++;
                    if (j==matrix_number) break;
               }
               if (j==matrix_number) break;
          }

          num_out_row=temp; 
     }

     matrix_out_row=matrix_number-num_out_row;

     //printf("myrank=%d,b_i_start=%d,b_i_end=%d,b_s=%d,b_e=%d,bsi=%d,bsi=%d,num_out_row=%d\n",\
     myrank,b_i_start,b_i_end,bound_edge[b_i_start],bound_edge[b_i_end],matrix_i[0],matrix_i[matrix_number-1],num_out_row);
     //MPI_Barrier(MPI_COMM_WORLD); 

     matrix_i_rr=(int*) malloc( (matrix_out_row)*sizeof(int) );
     matrix_j_rr=(int*) malloc( (matrix_out_row)*sizeof(int) );
     matrix_a_r_rr  =(double*) malloc( (matrix_out_row)*sizeof(double) );
     matrix_a_i_rr  =(double*) malloc( (matrix_out_row)*sizeof(double) );

     j=0;
     k=0;
     if (b_i_start==0 || b_i_start== bound_edge_total)
     {
          for (i=0;i<=matrix_number-1;i++)
          {
               matrix_i_rr[i]=matrix_i[i]-b_i_start;
               matrix_j_rr[i]=matrix_j[i];
               matrix_a_r_rr[i]=matrix_a_r[i];
               matrix_a_i_rr[i]=matrix_a_i[i];
          }   
     }
     else
     {
          for (i=b_i_start;i<=b_i_end;i++)
          {
               while (matrix_i[j]<bound_edge[i] )
               { 
                    matrix_i_rr[k]=matrix_i[j]-i+1;
                    matrix_j_rr[k]=matrix_j[j];
                    matrix_a_r_rr[k]=matrix_a_r[j];
                    matrix_a_i_rr[k]=matrix_a_i[j];
                    j++;
                    k++;
                    if (j==matrix_number) break;
               }

               if (j==matrix_number) break;
 
               while (matrix_i[j]==bound_edge[i] )
               {
                    j++;
                    if (j==matrix_number) break;
               }

               if (j==matrix_number) break;

               while (matrix_i[j]>bound_edge[b_i_end] )
               { 
                    matrix_i_rr[k]=matrix_i[j]-b_i_end;
                    matrix_j_rr[k]=matrix_j[j];
                    matrix_a_r_rr[k]=matrix_a_r[j];
                    matrix_a_i_rr[k]=matrix_a_i[j];
                    j++;
                    k++;
                    if (j==matrix_number) break;
               }
               if (j==matrix_number) break;
          }
     }

     free(matrix_i);
     free(matrix_j);
     free(matrix_a_r);
     free(matrix_a_i);
     
     /*if (myrank==1)
     {
        for (i=0;i<=matrix_out_row-1;i++)
            printf("i=%d,j=%d\n",matrix_i_rr[i],matrix_j_rr[i]);
     }*/

     /*********************remove column**********************************/ 

     quick_sort_2D_double(matrix_j_rr,matrix_i_rr,matrix_a_r_rr, matrix_a_i_rr,0,matrix_out_row-1);

     if (matrix_j_rr[0]> bound_edge[bound_edge_total])
     {
          b_j_start=bound_edge_total;
          b_j_end  =bound_edge_total;
     }
     else if (matrix_j_rr[matrix_out_row-1] < bound_edge[1])
     {
          b_j_start=0;
          b_j_end  =0; 
     }
     else
     {
          for (i=1;i<=bound_edge_total;i++)
          {
               if (matrix_j_rr[0]<=bound_edge[i]) 
               {    
                    b_j_start=i;
                    break;
               }
          }

          for (i=1;i<=bound_edge_total;i++)
          {
               if (matrix_j_rr[matrix_out_row-1]<=bound_edge[i])
               {    
                    b_j_end=i;
                    break;
               }
          }
          if (matrix_j_rr[matrix_out_row-1]>=bound_edge[bound_edge_total])
               b_j_end=bound_edge_total;
     }
    
     temp=0;
     j=0;
     if (b_j_start==0 || b_j_start== bound_edge_total)
     {
         num_out_column=0;
     }
     else
     {
          for (i=b_j_start;i<=b_j_end;i++)
          {
               while (matrix_j_rr[j] <bound_edge[i] )
               {
                    j++;
                    if (j==matrix_out_row) break;
               }

               if (j==matrix_out_row) break;
  
               while (matrix_j_rr[j]==bound_edge[i] )
               {
                    j++;
                    temp++;
                    if (j==matrix_out_row) break;               
               }
               if (j==matrix_out_row) break;
          }

          num_out_column=temp; 
     }

     //printf("myrank=%d,b_j_start=%d,b_j_end=%d,b_s=%d,b_e=%d,bsj=%d,bsj=%d,num_out_row=%d\n",\
     myrank,b_j_start,b_j_end,bound_edge[b_j_start],bound_edge[b_j_end],matrix_j_rr[0],matrix_j_rr[matrix_out_row-1],num_out_column);
     //MPI_Barrier(MPI_COMM_WORLD); 
     //exit(0) ;
     
     matrix_out_rc=matrix_out_row-num_out_column;
     matrix_number=matrix_out_row-num_out_column;

     matrix_i=(int*) malloc( (matrix_number)*sizeof(int) );
     matrix_j=(int*) malloc( (matrix_number)*sizeof(int) );
     matrix_a_r  =(double*) malloc( (matrix_number)*sizeof(double) );
     matrix_a_i  =(double*) malloc( (matrix_number)*sizeof(double) );
     
     matrix_i_rh=(int*) malloc( (num_out_column)*sizeof(int) );
     matrix_j_rh=(int*) malloc( (num_out_column)*sizeof(int) );
     matrix_index_rh=(int*) malloc( (num_out_column)*sizeof(int) );
     matrix_a_r_rh  =(double*) malloc( (num_out_column)*sizeof(double) );
     matrix_a_i_rh  =(double*) malloc( (num_out_column)*sizeof(double) );

     //printf("sssssssssss matrix_out_row=%d,num_out_row=%d,matrix_out_rc=%d,num_out_column=%d\n",matrix_out_row,num_out_row,matrix_out_rc,num_out_column);

     j=0;
     k=0;
     l=0;

     if (b_j_start==0 || b_j_start == bound_edge_total)
     {
          for (i=0;i<=matrix_out_row-1;i++)
          {
               matrix_i[i]=matrix_i_rr[i];
               matrix_j[i]=matrix_j_rr[i]-b_j_start;
               matrix_a_r[i]=matrix_a_r_rr[i];
               matrix_a_i[i]=matrix_a_i_rr[i];
          }   
     }
     else
     {
          for (i=b_j_start;i<=b_j_end;i++)
          {
               while (matrix_j_rr[j] <bound_edge[i] )
               { 
                    matrix_i[k]=matrix_i_rr[j];
                    matrix_j[k]=matrix_j_rr[j]-i+1;
                    matrix_a_r[k]=matrix_a_r_rr[j];
                    matrix_a_i[k]=matrix_a_i_rr[j];
                    j++;
                    k++;
                    if (j==matrix_out_row) break;
               }
               if (j==matrix_out_row) break;
 
               while (matrix_j_rr[j]==bound_edge[i] )
               {
                    matrix_i_rh[l]=matrix_i_rr[j];
                    matrix_j_rh[l]=matrix_j_rr[j];
                    matrix_a_r_rh[l]  =matrix_a_r_rr[j];
                    matrix_a_i_rh[l]  =matrix_a_i_rr[j];
                    matrix_index_rh[l]=i;
                    //i_rh -> b_i
                    //index_rh -> bound_edge[]
                    l++;
                    j++;
                    if (j==matrix_out_row) break;
               }

               if (j==matrix_out_row) break;

               while (matrix_j_rr[j]>bound_edge[b_j_end] )
               { 
                    matrix_i[k]=matrix_i_rr[j];
                    matrix_j[k]=matrix_j_rr[j]-b_j_end;
                    matrix_a_r[k]=matrix_a_r_rr[j];
                    matrix_a_i[k]=matrix_a_i_rr[j];
                    j++;
                    k++;
                    if (j==matrix_out_row) break;
               }
               if (j==matrix_out_row) break;
          }
     }

     free(matrix_i_rr);
     free(matrix_j_rr);
     free(matrix_a_r_rr);
     free(matrix_a_i_rr);

     /*if (myrank==0)
     {
        for (i=0;i<=matrix_number-1;i++)
            printf("i=%d,j=%d\n",matrix_i[i],matrix_j[i]);
     }*/
     
     quick_sort_2D_double(matrix_i,matrix_j,matrix_a_r,matrix_a_i,0,matrix_number-1);

         
     MPI_Barrier(MPI_COMM_WORLD);

     if (myrank==0)
     {
          printf("---------Matrix_Deform_Done-------------\n"); 
     }
     
}
//-------------------------------------------------------------------------------------------
// Generate_SLU_NR
//-------------------------------------------------------------------------------------------
void Generate_SLU_NR()
{
     int i,j;
     int temp_s,temp_e;

     nnz_loc=matrix_number;
     m_loc  =matrix_i[matrix_number-1]-matrix_i[0]+1;
     n_loc  =edge_number_total-bound_edge_total;
 
     //printf("1111111m_loc=%d,n_loc=%d,matrix_i[matrix_number-1]=%d,matrix_i[0]=%d\n",m_loc,n_loc,matrix_i[matrix_number-1],matrix_i[0]);

     fst_row=matrix_i[0]-1;

     matrix_xa   =(int*) malloc( (m_loc+1)*sizeof(int) );
     matrix_asub =(int*) malloc( (matrix_number)*sizeof(int) );

     temp_s=0;
     temp_e=0;

     for (i=0;i<=matrix_number-1;i++)
     {
          matrix_asub[i]=matrix_j[i]-1;
     }

     matrix_xa[0]=0;
     matrix_xa[m_loc]=matrix_number;
     j=0;

     for (i=0;i<=matrix_number-2;i++)
     {
         if (matrix_i[i]!=matrix_i[i+1])
         {
              j++;
              matrix_xa[j]=i+1;
         }
     }

     free(matrix_i);
     free(matrix_j);
        
     MPI_Barrier(MPI_COMM_WORLD);
     if (myrank==0)
     {
          printf("---------Generate_SLU_N_Done------------\n"); 
     }

}
//------------------------------------------------------------------------------------------
// quick_sort_2D
//------------------------------------------------------------------------------------------
void quick_sort_2D_double(int *e1, int *e2, double *n1, double *n2, int low, int high)
{
     
     int privotLoc;
     if(low < high)
     {    
          privotLoc = partition_double(e1,e2,n1,n2,low,high);
          
          quick_sort_2D_double(e1,e2,n1,n2,low,privotLoc -1); 
          quick_sort_2D_double(e1,e2,n1,n2,privotLoc + 1, high);
     }
}
int partition_double(int *e1, int *e2, double *n1, double *n2,int low, int high)
{    
     int privotKey1 = e1[low];
     int privotKey2 = e2[low];
       
     while(low < high)
     {
          
          while(low < high  && \
          (e1[high] >  privotKey1 || (e1[high] == privotKey1 && e2[high] >= privotKey2)))\
          --high;
          {     
               swap(&e1[low], &e1[high]);
               swap(&e2[low], &e2[high]);
               swap_double(&n1[low], &n1[high]);
               swap_double(&n2[low], &n2[high]);
          }
          while(low < high  && \
          (e1[low]  <  privotKey1 || (e1[low]  == privotKey1 && e2[low] <= privotKey2)))\
          ++low;
          {
               swap(&e1[low], &e1[high]);
               swap(&e2[low], &e2[high]);
               swap_double(&n1[low], &n1[high]);
               swap_double(&n2[low], &n2[high]);
          }
     }
     return low;
}

void swap_double(double *a, double *b)
{
     double tmp = *a;
     *a = *b;
     *b = tmp;
}

int inverseMatrix(double a[3][3],double inv[3][3])
{
     double temp[3][2*3];

     //将原矩阵和单位矩阵拼接在一起
     for(int i=0;i<3;i++)
     {
          for(int j=0;j<3;j++)
          {
               temp[i][j] = a[i][j];
               temp[i][j+3] = (i == j) ? 1.0 : 0.0;
          }
     }

     //应用高斯-约旦消去法
     for(int i=0;i<3;i++)
     {
          if(temp[i][i]==0.0)
          {
               printf("无法求逆:主元为零\n");
               return 0;
          }
          for(int j=0;j<3;j++)
          {
               if(i!=j)
               {
                    double ratio = temp[j][i] / temp[i][i];
                    for(int k=0;k<6;k++)
                    {
                         temp[j][k] -= ratio*temp[i][k];
                    }
               }
          }
     }

     //生成逆矩阵
     for(int i=0;i<3;i++)
     {
          for(int j=0;j<3;j++)
          {
               inv[i][j] = temp[i][j+3] / temp[i][i];
          }
     }

     return 1;
}
//------------------------------------------------------------------------------------------

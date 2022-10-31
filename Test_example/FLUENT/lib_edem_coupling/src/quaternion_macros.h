#ifndef QUATERNION_MACROS_H
# define QUATERNION_MACROS_H 1

/* Quaternion macros */
#ifndef ND_Q
# define ND_Q 4
#endif

#define QN_S(Q,EQ,P)((Q)[0]EQS(),(Q)[1]EQ(S),(Q)[2]EQ(S),(Q)[3]EQ(S))
#define QN_Q(Q,EQ,P)((Q)[0]EQ(P)[0],(Q)[1]EQ(P)[1],(Q)[2]EQ(P)[2],(Q)[3]EQ(P)[3])
#define QN_D(Q,EQ,PR,PI,PJ,PK)((Q)[0]EQ(PR),(Q)[1]EQ(PI),(Q)[2]EQ(PJ),(Q)[3]EQ(PK))
#define QD_Q(QR,QI,QJ,QK,EQ,P)((QR)EQ(P)[0],(QI)EQ(P)[1],(QJ)EQ(P)[2],(QK)EQ(P)[3])
#define QD_D(QR,QI,QJ,QK,EQ,PI,PJ,PK)((QR)EQ(PR),(QI)EQ(PI),(QJ)EQ(PJ),(QK)EQ(PK))
#define QN_V(Q,EQ,V)QN_D(Q,EQ,0.0,(V)[0],(V)[1],(V)[2])
#define NV_Q(V,EQ,Q)NV_D(V,EQ,(Q)[1],(Q)[2],(Q)[3])
#define QD_MAG2(QR,QI,QJ,QK)((QR)*(QR)+(QI)*(QI)+(QJ)*(QJ)+(QK)*(QK))
#define QD_MAG(QR,QI,QJ,QK)sqrt(QD_MAG2(QR,QI,QJ,QK))
#define QN_MAG2(Q)((Q[0])*(Q[0])+(Q[1])*(Q[1])+(Q[2])*(Q[2])+(Q[3])*(Q[3]))
#define QN_MAG(Q)sqrt(QN_MAG2(Q))

#define QN_CONJ(QC,Q)((QC)[0]=(Q)[0],(QC)[1]=-(Q)[1],(QC)[2]=-(Q)[2],(QC)[3]=-(Q)[3])

#define QN_MUL(Q,A,B)((Q)[0]=(((A)[0]*(B)[0])-((A)[1]*(B)[1])-((A)[2]*(B)[2])-((A)[3]*(B)[3])),\
                      (Q)[1]=(((A)[1]*(B)[0])+((A)[0]*(B)[1])+((A)[2]*(B)[3])-((A)[3]*(B)[2])),\
                      (Q)[2]=(((A)[2]*(B)[0])+((A)[0]*(B)[2])+((A)[3]*(B)[1])-((A)[1]*(B)[3])),\
                      (Q)[3]=(((A)[3]*(B)[0])+((A)[0]*(B)[3])+((A)[1]*(B)[2])-((A)[2]*(B)[1])))

#endif /* QUATERNION_MACROS_H */

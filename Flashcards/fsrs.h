#ifndef FSRS_H
#define FSRS_H

double fsrs_retrievability(double t, double s);
double fsrs_interval(double r, double s);
double fsrs_s_0(int g);
double fsrs_stability(double r, double s, double d, int g);
double fsrs_d_0(int g);
double fsrs_difficulty(double d, int g);

#endif // FSRS_H
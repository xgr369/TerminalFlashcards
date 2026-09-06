#include "fsrs.h"
#include <math.h>
#include <assert.h>

#define F (19.0 / 81.0)
#define C (-0.5)
#define GRADE_FORGOT 0
#define GRADE_HARD 1
#define GRADE_GOOD 2
#define GRADE_EASY 3

static const double W[19] = {
	0.40255, 1.18385, 3.173, 15.69105, 7.1949, 0.5345, 1.4604, 0.0046, 1.54575, 0.1192, 1.01925,
	1.9395, 0.11, 0.29605, 2.2698, 0.2315, 2.9898, 0.51655, 0.6621,
};

double fsrs_retrievability(double t, double s) {
	return pow(1.0 + F * (t / s), C);
}

double fsrs_interval(double r, double s) {
	return (s / F) * (pow(r, 1.0 / C) - 1.0);
}

double fsrs_s_0(int g) {
	switch (g) {
		case 0:
			return W[0];
		case 1:
			return W[1];
		case 2:
			return W[2];
		case 3:
			return W[3];
	}
	assert(0);
}

static double s_success(double r, double s, double d, int g) {
	double t_d = 11.0 - d;
	double t_s = pow(s, -W[9]);
	double t_r = exp(W[10] * (1.0 - r)) - 1.0;
	double h = g == 1 ? W[15] : 1.0;
	double b = g == 3 ? W[16] : 1.0;
	double c = exp(W[8]);
	double alpha = 1.0 + t_d * t_s * t_r * h * b * c;
	return s * alpha;
}

static double s_fail(double r, double s, double d) {
	double d_f = pow(d, -W[12]);
	double s_f = pow(s + 1.0, W[13]) - 1.0;
	double r_f = exp(W[14] * (1.0 - r));
	double c_f = exp(W[11]);
	s_f = d_f * s_f * r_f * c_f;
	return fmin(s_f, s);
}

double fsrs_stability(double r, double s, double d, int g) {
	return g == 0 ? s_fail(r, s, d) : s_success(r, s, d, g);
}

static double clamp_d(double d) {
	if (d < 1.0) {
		return 1.0;
	}
	if (d > 10.0) {
		return 10.0;
	}
	return d;
}

double fsrs_d_0(int g) {
	return clamp_d(W[4] - exp(W[5] * (g - 1.0)) + 1.0);
}

static double delta_d(int g) {
	return -W[6] * (g - 1.0);
}

static double dp(double d, int g) {
	return d + delta_d(g) * ((10.0 - d) / 9.0);
}

double fsrs_difficulty(double d, int g) {
	return clamp_d(W[7] * fsrs_d_0(GRADE_EASY) + (1.0 - W[7]) * dp(d, g));
}

/* ***************************************************************************
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  As a special exception, you may use this file as part of a free software
 *  library without restriction.  Specifically, if other files instantiate
 *  templates or use macros or inline functions from this file, or you compile
 *  this file and link it with other files to produce an executable, this
 *  file does not by itself cause the resulting executable to be covered by
 *  the GNU General Public License.  This exception does not however
 *  invalidate any other reasons why the executable file might be covered by
 *  the GNU General Public License.
 *
 ****************************************************************************
 */

/*

   Author: Marco Aldinucci.
   email:  aldinuc@di.unipi.it
   marco@pisa.quadrics.com
   date :  15/11/97

Modified by:

****************************************************************************
 *  Author: Dalvan Griebler <dalvangriebler@gmail.com>
 *
 *  Copyright: GNU General Public License
 *  Description: This program simply computes the mandelbroat set.
 *  File Name: mandel.cpp
 *  Version: 1.0 (25/05/2018)
 *  Compilation Command: make
 ****************************************************************************
*/


#include <stdio.h>
#include "marX2.h"
#include <sys/time.h>
#include <math.h>

#include <iostream>
#include <chrono>

#include <tbb/task_scheduler_init.h>
#include <tbb/parallel_for.h>
#include <tbb/pipeline.h>

#define DIM 800
#define ITERATION 1024

double diffmsec(struct timeval  a,  struct timeval  b) {
    long sec  = (a.tv_sec  - b.tv_sec);
    long usec = (a.tv_usec - b.tv_usec);

    if(usec < 0) {
        --sec;
        usec += 1000000;
    }
    return ((double)(sec*1000)+ (double)usec/1000.0);
}

struct Stage1Result {
    int i;
    double im;
};

class stage1: public tbb::filter {
public:
    int i;
    int dim, niter;
    double init_a, init_b, range, step;
	stage1(int dim0, int niter0, double init_a0, double init_b0, double range0):tbb::filter(tbb::filter::serial) {
        dim = dim0;
        niter = niter0;
        init_a = init_a0;
        init_b = init_b0;
        range = range0;
        step = range/((double) dim);
        i=0;
	}
	void* operator() (void*){
        if (i<dim) {
            Stage1Result *it = new Stage1Result;
            it->i=i;
            it->im=init_b+(step*i);

            i++;
            return it;
        }
		return NULL;
	}
};

struct Stage2Result {
    int i;
    unsigned char *M;
};

class stage2: public tbb::filter {
public:
    int dim, niter;
    double init_a, init_b, range, step;
	stage2(int dim0, int niter0, double init_a0, double init_b0, double range0):tbb::filter(tbb::filter::serial) {
        dim = dim0;
        niter = niter0;
        init_a = init_a0;
        init_b = init_b0;
        range = range0;
        step = range/((double) dim);
	}
	void* operator() (void* item){
		Stage1Result *it = static_cast <Stage1Result*> (item);
        Stage2Result *result = new Stage2Result;
        result->M = (unsigned char *) malloc(dim);
        result->i = it->i;

        double init_a0, step0;
        int niter0;
        init_a0 = init_a;
        step0 = step;
        niter0 = niter;

        // for (j=0; j<dim; j++) {
        tbb::parallel_for(0,dim,[init_a0, step0, niter0, it, result](int j){
            double a,b,a2,b2,cr;
            int k;
            a=cr=init_a0+step0*j;
            b=it->im;
            k=0;
            for (k=0; k<niter0; k++)
            {
                a2=a*a;
                b2=b*b;
                if ((a2+b2)>4.0) break;
                b=2*a*b+it->im;
                a=a2-b2+cr;
            }
            result->M[j]= (unsigned char) 255-((k*255/niter0));
        // }
        });
        delete it;

		return result;
	}
};

class stage3: public tbb::filter {
public:
    int dim, niter;
    double init_a, init_b, range, step;
	stage3(int dim0, int niter0, double init_a0, double init_b0, double range0):tbb::filter(tbb::filter::serial) {
        dim = dim0;
        niter = niter0;
        init_a = init_a0;
        init_b = init_b0;
        range = range0;
        step = range/((double) dim);
	}
	void* operator() (void* item){
		Stage2Result *it = static_cast <Stage2Result*> (item);

#if !defined(NO_DISPLAY)
        ShowLine(it->M,dim,it->i);
#endif

        delete it;

		return NULL;
	}
};

int main(int argc, char **argv) {
    std::cout << "#pipeline(seq,map(seq),seq) pattern implementation!!" << std::endl;
    double init_a=-2.125,init_b=-1.5,range=3.0;
    int dim = DIM, niter = ITERATION;
    // stats
    struct timeval t1,t2;
    int r,retries=1;
    double avg=0, var, * runs;

    int threads = 4;
    char *threads_env = getenv("THREADS");
    if (threads_env != NULL) {
        threads = atoi(threads_env);
    }
    printf("threads: %d\n", threads);

    if (argc<3) {
        printf("Usage: seq size niterations\n\n\n");
    }
    else {
        dim = atoi(argv[1]);
        niter = atoi(argv[2]);
        retries = atoi(argv[3]);
    }
    runs = (double *) malloc(retries*sizeof(double));

    printf("Mandebroot set from (%g+I %g) to (%g+I %g)\n",
           init_a,init_b,init_a+range,init_b+range);
    printf("resolution %d pixel, Max. n. of iterations %d\n",dim*dim,ITERATION);

#if !defined(NO_DISPLAY)
    SetupXWindows(dim,dim,1,NULL,"Sequential Mandelbroot");
#endif

    for (r=0; r<retries; r++) {

        // Start time
        gettimeofday(&t1,NULL);

        tbb::task_scheduler_init init(threads);
        tbb::pipeline pipeline;
        stage1 stg1(dim, niter, init_a, init_b, range);
        pipeline.add_filter(stg1);
        stage2 stg2(dim, niter, init_a, init_b, range);
        pipeline.add_filter(stg2);
        stage3 stg3(dim, niter, init_a, init_b, range);
        pipeline.add_filter(stg3);
        pipeline.run(threads);
        pipeline.clear();

        // Stop time
        gettimeofday(&t2,NULL);

        avg += runs[r] = diffmsec(t2,t1);
        printf("Run [%d] DONE, time= %f (ms)\n",r, runs[r]);
    }
    avg = avg / (double) retries;
    var = 0;
    for (r=0; r<retries; r++) {
        var += (runs[r] - avg) * (runs[r] - avg);
    }
    var /= retries;
    printf("Average on %d experiments = %f (ms) Std. Dev. %f\n\nPress a key\n",retries,avg,sqrt(avg));

#if !defined(NO_DISPLAY)
    getchar();
    CloseXWindows();
#endif

    return 0;
}

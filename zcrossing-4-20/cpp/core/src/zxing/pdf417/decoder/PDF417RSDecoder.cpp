/* Copyright 2011 ZXing authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * 
 * Ported from:
 *
 *  Pdf417decode.c
 *
 *   Original software by Ian Goldberg, 
 *   Modified and updated by OOO S.  (ooosawaddee3@hotmail.com)
 *   Version 2.0 by Hector Peraza (peraza@uia.ua.ac.be)
 *
 *   History:
 *
 *   22/12/01  pdf417decode             Initial Release
 *   23/12/01  pdf417decode rev 1.1     Added a test to see if fopen() suceeded.
 *   07/03/04  pdf417decode rev 2.0     Decoding routines heavily modified.
 *                                      Fixed start-of-row symbol problem.
 *                                      Numeric and text compactions now supported.
 * 2012-06-05 PDF417RSDecoder.cpp HFN   ported from Java into C++ (for Windows)
 * 2012-06-27 PDF417RSDecoder           some renames (Alpha_to_ ==> Powers_Of_3,
 *                                      Index_of_ ==> Discrete_Log_3)
 *                                      some debug msgs added
 */
#include <zxing/pdf417/decoder/PDF417RSDecoder.h>


namespace zxing {
namespace pdf417 {

using namespace std;

  const int PDF417RSDecoder::NN = 1024;
  const int PDF417RSDecoder::PRIM = 1;
  const int PDF417RSDecoder::GPRIME = 929;
  const int PDF417RSDecoder::KK = 32;
  const int PDF417RSDecoder::A0 = 928;

  /**
   * initialize table of 3^i for syndrome calculation
   */
  void PDF417RSDecoder::powers_init() {
    int ii;
    int power_of_3;
	
	if(rs_init_)
	  return;
	
	/* The prime number GPRIME = 929 has the property that the set of
	* numbers (3 ** n) MOD GPRIME, n = 0 ... GPRIME-2, contains all numbers from 1
	* to (GPRIME-1). This is true f.ex. for GPRIME = 7 or 17, but not for 11 or 13,
	* for instance. So it will be possible to define the inverted function of "Powers_Of_3"
	* that is also known as the "discrete logarithm of Base 3" (that is, "y = (3 ** x) MOD 929").
	*/
    power_of_3 = 1;
    // Discrete_Log_3[1] = GPRIME - 1; superfluous??

    for (ii = 0; ii < GPRIME - 1; ii += 1) {
      Powers_Of_3[ii] = power_of_3;
      if (power_of_3 < GPRIME) { /* always the case */
        if (ii != GPRIME - 1)    /* ii does not reach GPRIME-1 */
          Discrete_Log_3[power_of_3] = ii;
      } 
#if (defined (DEBUG))
	  else { /* should be never reached */
        print("Internal error: powers of 3 calculation");
      }
#endif

      power_of_3 = (power_of_3 * 3) % GPRIME;
    }
    Discrete_Log_3[0] = GPRIME - 1;
    Powers_Of_3[GPRIME - 1] = 1;
    Discrete_Log_3[GPRIME] = A0;
	
	rs_init_ = true;
  }

  PDF417RSDecoder::PDF417RSDecoder() {
	rs_init_ = false;
	Powers_Of_3 = new Array<int>(1024);
	Discrete_Log_3 = new Array<int>(1024);
    powers_init();
  }
  
  PDF417RSDecoder::~PDF417RSDecoder() {
  }

  int PDF417RSDecoder::modbase(int x) {
    return ((x) % (GPRIME - 1));
  }

  /*
   * int data[1024], int eras_pos[1024 - 32]
   * 
   * Performs ERRORS+ERASURES decoding of RS codes. If decoding is successful,
   * writes the codeword into data[] itself. Otherwise data[] is unaltered.
   * 
   * Return number of symbols corrected, or -1 if codeword is illegal or
   * uncorrectable. If eras_pos is non-null, the detected error locations are
   * written back. NOTE! This array must be at least NN-KK elements long.
   * 
   * First "no_eras" erasures are declared by the calling program. Then, the
   * maximum # of errors correctable is t_after_eras = floor((NN-KK-no_eras)/2).
   * If the number of channel errors is not greater than "t_after_eras" the
   * transmitted codeword will be recovered. Details of algorithm can be found
   * in R. Blahut's "Theory ... of Error-Correcting Codes".
   * 
   * Warning: the eras_pos[] array must not contain duplicate entries; decoder
   * failure will result. The decoder *could* check for this condition, but it
   * would involve extra time on every decoding operation.
   */

  int PDF417RSDecoder::correctErrors(ArrayRef<int>data, ArrayRef<int>eras_pos, int no_eras, int data_len, int synd_len) {
    int deg_lambda, el, deg_omega;

#if (defined DEBUG)
	char szmsg[256];
#endif

#if (defined (DEBUG))
	sprintf(szmsg,"PDF417RSDecoder::correctErrors(no_eras=%d,data_len=%d,synd_len=%d)",no_eras,data_len,synd_len);
	print(szmsg);
#endif
    int i, j, r, k;
    int u, q, tmp, num1, num2, den, discr_r;
    ArrayRef<int> lambda = new Array<int>(2048 + 1); ArrayRef<int> s = new Array<int>(2048 + 1); /*
                                                              * Err+Eras Locator
                                                              * poly and
                                                              * syndrome poly
                                                              */
    ArrayRef<int> b = new Array<int>(2048 + 1); ArrayRef<int> t = new Array<int>(2048 + 1); ArrayRef<int> omega = new Array<int>(2048 + 1);
    ArrayRef<int> root = new Array<int>(2048); ArrayRef<int> reg = new Array<int>(2048 + 1); ArrayRef<int> loc = new Array<int>(2048);
    int syn_error;
    int count;
    int ci;
    int error_val;
    int fix_loc;
	bool is_finished = false;

    if (!rs_init_)
      powers_init();

    /* finish: */
	/* 2012-06-08 HFN in order to avoid some goto's .... */
	while (!is_finished) {
      /*
       * form the syndromes; i.e. evaluate data(x) at roots of g(x) namely
       * @**(1+i)*PRIM, i = 0, ... , (NN-KK-1)
       */
      for (i = 1; i <= synd_len; i++) {
        s[i] = 0;// data[data_len];
      }

      for (j = 1; j <= data_len; j++) {

        if (data[data_len - j] == 0)
          continue;

        tmp = Discrete_Log_3[data[data_len - j]];

        /* s[i] ^= Powers_Of_3[modbase(tmp + (1+i-1)*j)]; */
        for (i = 1; i <= synd_len; i++) {
          s[i] = (s[i] + Powers_Of_3[modbase(tmp + (i) * j)]) % GPRIME;
        }
      }

      /* Convert syndromes to index form, checking for nonzero condition */
      syn_error = 0;
      for (i = 1; i <= synd_len; i++) {
        syn_error |= s[i];
        s[i] = Discrete_Log_3[s[i]];
      }

      if (syn_error == 0) {
        /*
         * if syndrome is zero, data[] is a codeword and there are no errors to
         * correct. So return data[] unmodified
         */
        count = 0;
#if (defined (DEBUG))
        print("No errors");
#endif
		is_finished = true;
        break /* finish */ ;
      }

      for (ci = synd_len - 1; ci >= 0; ci--)
        lambda[ci + 1] = 0;

      lambda[0] = 1;

      if (no_eras > 0) {
        /* Init lambda to be the erasure locator polynomial */
        lambda[1] = Powers_Of_3[modbase(PRIM * eras_pos[0])];
        for (i = 1; i < no_eras; i++) {
          u = modbase(PRIM * eras_pos[i]);
          for (j = i + 1; j > 0; j--) {
            tmp = Discrete_Log_3[lambda[j - 1]];
            if (tmp != A0)
              lambda[j] = (lambda[j] + Powers_Of_3[modbase(u + tmp)]) % GPRIME;
          }
        }
      }
      for (i = 0; i < synd_len + 1; i++)
        b[i] = Discrete_Log_3[lambda[i]];

      /*
       * Begin Berlekamp-Massey algorithm to determine error+erasure locator
       * polynomial
       */
      r = no_eras;
      el = no_eras;
      while (++r <= synd_len) { /* r is the step number */
        /* Compute discrepancy at the r-th step in poly-form */
        discr_r = 0;
        for (i = 0; i < r; i++) {
          if ((lambda[i] != 0) && (s[r - i] != A0)) {
            if (i % 2 == 1) {
              discr_r = (discr_r + Powers_Of_3[modbase((Discrete_Log_3[lambda[i]] + s[r - i]))]) % GPRIME;
            } else {
              discr_r = (discr_r + GPRIME - Powers_Of_3[modbase((Discrete_Log_3[lambda[i]] + s[r - i]))])
                  % GPRIME;
            }
          }
        }

        discr_r = Discrete_Log_3[discr_r]; /* Index form */

        if (discr_r == A0) {
          /* 2 lines below: B(x) <-- x*B(x) */
          // COPYDOWN(&b[1],b,synd_len);
          //
          for (ci = synd_len - 1; ci >= 0; ci--)
            b[ci + 1] = b[ci];
          b[0] = A0;
        } else {
          /* 7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x) */
          /* the T(x) will become the next lambda */

          t[0] = lambda[0];
          for (i = 0; i < synd_len; i++) {
            if (b[i] != A0) {

              // t[i+1] = (lambda[i+1] + GPRIME -
              // Powers_Of_3[modbase(discr_r + GPRIME - 1 - b[i])]) % GPRIME;
              t[i + 1] = (lambda[i + 1] + Powers_Of_3[modbase(discr_r + b[i])]) % GPRIME;

            } else {
              t[i + 1] = lambda[i + 1];
            }
          }
          el = 0;
          if (2 * el <= r + no_eras - 1) {
            el = r + no_eras - el;
            /*
             * 2 lines below: B(x) <-- inv(discr_r) * lambda(x)
             */
            for (i = 0; i <= synd_len; i++) {

              if (lambda[i] == 0) {
                b[i] = A0;
              } else {
                b[i] = modbase(Discrete_Log_3[lambda[i]] - discr_r + GPRIME - 1);
              }
            }

          } else {
            /* 2 lines below: B(x) <-- x*B(x) */
            // COPYDOWN(&b[1],b,synd_len);
            for (ci = synd_len - 1; ci >= 0; ci--)
              b[ci + 1] = b[ci];
            b[0] = A0;
          }
          // COPY(lambda,t,synd_len+1);

          for (ci = synd_len + 1 - 1; ci >= 0; ci--) {
            lambda[ci] = t[ci];
          }
        }
      }

      /* Convert lambda to index form and compute deg(lambda(x)) */
      deg_lambda = 0;
      for (i = 0; i < synd_len + 1; i++) {

#if (defined (DEBUG))
			sprintf(szmsg,"correctErrors: i=%4d, lambda[i]=%4d, log3(lambda[i])=%4d",i,lambda[i],Discrete_Log_3[lambda[i]]);
            print(szmsg);
#endif
        lambda[i] = Discrete_Log_3[lambda[i]];

        if (lambda[i] != A0) {
          deg_lambda = i;
#if (defined (DEBUG))
			sprintf(szmsg,"correctErrors: deg_lambda=%4d",deg_lambda);
            print(szmsg);
#endif
		}
      }

      /*
       * Find roots of the error+erasure locator polynomial by Chien Search
       */

      for (ci = synd_len - 1; ci >= 0; ci--)
        reg[ci + 1] = lambda[ci + 1];

      count = 0; /* Number of roots of lambda(x) */
      for (i = 1, k = data_len - 1; i <= GPRIME; i++) {
        q = 1;
        for (j = deg_lambda; j > 0; j--) {

          if (reg[j] != A0) {
            reg[j] = modbase(reg[j] + j);
            // q = modbase( q + Powers_Of_3[reg[j]]);
            if (deg_lambda != 1) {
              if (j % 2 == 0) {
                q = (q + Powers_Of_3[reg[j]]) % GPRIME;
              } else {
                q = (q + GPRIME - Powers_Of_3[reg[j]]) % GPRIME;
              }
            } else {
              q = Powers_Of_3[reg[j]] % GPRIME;
              if (q == 1)
                --q;
            }
          }
        }

        if (q == 0) {
          /* store root (index-form) and error location number */
          root[count] = i;

          loc[count] = GPRIME - 1 - i;
          if (count < synd_len) {
            count += 1;
          }
#if (defined (DEBUG))
		  else {
			sprintf(szmsg,"Error : Error count too big = ",count);
            print(szmsg);
          }
#endif
        }
        if (k == 0) {
          k = data_len - 1;
        } else {
          k -= 1;
        }

        /*
         * If we've already found max possible roots, abort the search to save
         * time
         */

#if (defined (DEBUG) && defined(DEBUGALL))
		if(k==0||k==data_len-1) {
			sprintf(szmsg,"root count = %d, deg_lambda = %d, k=%d, q=%d",count,deg_lambda,k,q);
			print(szmsg);
		}
#endif
        if (count == deg_lambda)
          break;

      }

      if (deg_lambda != count) {
        /*
         * deg(lambda) unequal to number of roots => uncorrectable error
         * detected
         */

#if (defined (DEBUG))
		sprintf(szmsg,"Uncorrectable error: root count = %d, deg_lambda = %d",count,deg_lambda);
        print(szmsg);
#endif
        count = -1;
		is_finished = true;
        break /* finish */;
      }

      /*
       * Compute err+eras evaluator poly omega(x) = s(x)*lambda(x) (modulo
       * x**(synd_len)). in index form. Also find deg(omega).
       */
      deg_omega = 0;
      for (i = 0; i < synd_len; i++) {
        tmp = 0;
        j = (deg_lambda < i) ? deg_lambda : i;
        for (; j >= 0; j--) {
          if ((s[i + 1 - j] != A0) && (lambda[j] != A0)) {
            if (j % 2 != 0) {
              tmp = (tmp + GPRIME - Powers_Of_3[modbase(s[i + 1 - j] + lambda[j])]) % GPRIME;
            } else {

              tmp = (tmp + Powers_Of_3[modbase(s[i + 1 - j] + lambda[j])]) % GPRIME;
            }
          }
        }

        if (tmp != 0)
          deg_omega = i;
        omega[i] = Discrete_Log_3[tmp];

      }
      omega[synd_len] = A0;

      /*
       * Compute error values in poly-form. num1 = omega(inv(X(l))), num2 =
       * inv(X(l))**(B0-1) and den = lambda_pr(inv(X(l))) all in poly-form
       */
      for (j = count - 1; j >= 0 && !is_finished; j--) {
        num1 = 0;
        for (i = deg_omega; i >= 0; i--) {
          if (omega[i] != A0) {
            // num1 = ( num1 + Powers_Of_3[modbase(omega[i] + (i * root[j])]) %
            // GPRIME;
            num1 = (num1 + Powers_Of_3[modbase(omega[i] + ((i + 1) * root[j]))]) % GPRIME;
          }
        }
        // num2 = Powers_Of_3[modbase(root[j] * (1 - 1) + data_len)];

        num2 = 1;
        den = 0;

        // denominator if product of all (1 - Bj Bk) for k != j
        // if count = 1, then den = 1

        den = 1;
        for (k = 0; k < count; k += 1) {
          if (k != j) {
            tmp = (1 + GPRIME - Powers_Of_3[modbase(GPRIME - 1 - root[k] + root[j])]) % GPRIME;
            den = Powers_Of_3[modbase(Discrete_Log_3[den] + Discrete_Log_3[tmp])];
          }
        }

        if (den == 0) {
          /* Convert to dual- basis */
          count = -1;
		  is_finished = true;
          break /* finish */ ;
        }

        error_val = Powers_Of_3[modbase(Discrete_Log_3[num1] + Discrete_Log_3[num2] + GPRIME - 1 - Discrete_Log_3[den])]
            % GPRIME;

        /* Apply error to data */
        if (num1 != 0) {
          if (loc[j] < data_len + 1) {
            fix_loc = data_len - loc[j];
            if (fix_loc < data_len) {
#if (defined (DEBUG))
			  sprintf(szmsg,"PDF417RSDecoder::correctErrors: fix @ %d, original = %d after = %d",
				  fix_loc, data[fix_loc],(data[fix_loc] + GPRIME - error_val) % GPRIME);
              print(szmsg);
#endif
              data[fix_loc] = (data[fix_loc] + GPRIME - error_val) % GPRIME;
            }
          }
        }
		/* ... for finishing purposed added. */
		if(is_finished)
		  break;
      }

	  is_finished = true;	/* at this point we are finished */
    }
#if (defined (DEBUG))
	sprintf(szmsg,"PDF417RSDecoder::correctErrors: data_len = %d, count = %d",data_len,count);
    print(szmsg);
#endif
    return count;
  }

  void PDF417RSDecoder::print(char const* psc) {
    // System.out.println(string);
#if (defined _WIN32) /* Windows: TRACE for debug */
#if (defined (DEBUG))
	int szl = strlen(psc)+1;
	WCHAR *pwc = new WCHAR[szl];
	mbstowcs(pwc,psc,szl);
	OutputDebugString(pwc);
	OutputDebugString(L"\n");
	delete[] pwc;
#endif
#else
	cout << psc;
#endif
  }

} /* namespace pdf417 */
} /* namespace zxing */

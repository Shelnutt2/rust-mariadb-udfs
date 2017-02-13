/*
  Copyright Seth Shelnutt 2016-06-27
  Licensed Under the GPL v2 or later
  returns the summation of the values in a distribution
  input parameters:
  data (real)
  output:
  summation of the distribution (real)
  registering the function:
  CREATE AGGREGATE FUNCTION summation RETURNS REAL SONAME 'libsummation_udf.so';
  getting rid of the function:
  DROP FUNCTION summation;
*/

#ifdef STANDARD
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef __WIN__
typedef unsigned __int64 ulonglong;
typedef __int64 longlong;
#else
typedef unsigned long long ulonglong;
typedef long long longlong;
#endif /*__WIN__*/
#else
#include <my_global.h>
#include <my_sys.h>
#endif
#include <mysql.h>
#include <m_ctype.h>
#include <m_string.h>

#define BUFFERSIZE 1024

extern double rust_sum_float(double *input, unsigned long size);

#ifdef __cplusplus
extern "C" {
#endif
my_bool summation_init( UDF_INIT* initid, UDF_ARGS* args, char* message );
void summation_deinit( UDF_INIT* initid );
void summation_add( UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error );
void summation_clear( UDF_INIT* initid, char* is_null, char* is_error );
void summation_reset( UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error );
double summation( UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error );
#ifdef __cplusplus
}
#endif

typedef struct summation_data_struct
{
  unsigned long count;
  unsigned long abscount;
  unsigned long pages;
  double *values;
} summation_data;


my_bool summation_init( UDF_INIT* initid, UDF_ARGS* args, char* message )
{
  if (args->arg_count != 1)
  {
    strcpy(message,"wrong number of arguments: summation() requires one argument");
    return 1;
  }

  if (args->arg_type[0]!=REAL_RESULT && args->arg_type[0]!=INT_RESULT && args->arg_type[0] != DECIMAL_RESULT)
  {
    if (args->arg_type[0] == STRING_RESULT)
      strcpy(message,"summation() requires a real or integer as parameter 1, received STRING");
    else
      strcpy(message,"summation() requires a decimal, real or integer as parameter 1");
    return 1;
  }

  initid->decimals = NOT_FIXED_DEC;
  initid->maybe_null = 1;

  summation_data *buffer = malloc(sizeof(struct summation_data_struct));
  buffer->count = 0;
  buffer->abscount=0;
  buffer->pages = 1;
  buffer->values = NULL;

  //initid->max_length	= 32;
  initid->ptr = (void*)buffer;

  return 0;
}

void summation_deinit( UDF_INIT* initid )
{
  summation_data *buffer = (summation_data*)initid->ptr;

  if (buffer->values != NULL)
  {
    free(buffer->values);
    buffer->values=NULL;
  }
  free(initid->ptr);
}

void summation_add( UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* is_error )
{
  if (args->args[0]!=NULL)
  {
    summation_data *buffer = (summation_data*)initid->ptr;
    if (buffer->count>=BUFFERSIZE)
    {
      buffer->pages++;
      buffer->count=0;
      buffer->values=(double *) realloc(buffer->values,BUFFERSIZE*buffer->pages*sizeof(double));
    }
    if (args->arg_type[0]==REAL_RESULT)
      buffer->values[buffer->abscount++] = *((double*)args->args[0]);
    else if (args->arg_type[0]==DECIMAL_RESULT)
      buffer->values[buffer->abscount++] = strtod((char*)args->args[0], NULL);
    else if (args->arg_type[0]==INT_RESULT)
      buffer->values[buffer->abscount++] = (double)*((longlong*)args->args[0]);

    buffer->count++;
  }
}

void summation_clear( UDF_INIT* initid, char* is_null, char* is_error )
{
  summation_data *buffer = (summation_data*)initid->ptr;
  buffer->count = 0;
  buffer->abscount=0;
  buffer->pages = 1;
  *is_null = 0;
  *is_error = 0;

  if (buffer->values != NULL)
  {
    free(buffer->values);
    buffer->values=NULL;
  }

  buffer->values=(double *) malloc(BUFFERSIZE*sizeof(double));
}

void summation_reset( UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* is_error )
{
  summation_clear( initid, is_null, is_error );
  summation_add( initid, args, is_null, is_error );
}

double summation( UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* is_error )
{
  summation_data* buffer = (summation_data*)initid->ptr;
  *is_null=0;
  return rust_sum_float(buffer->values,buffer->abscount);
}

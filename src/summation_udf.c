/*
  Copyright Seth Shelnutt 2016-06-27
  Licensed Under the GPL v2 or later
  returns the summation of the values in a distribution
  input parameters:
  data (real)
  output:
  summation of the distribution (real)
  registering the function:
  CREATE AGGREGATE FUNCTION rsum RETURNS REAL SONAME 'libsummation_udf.so';
  getting rid of the function:
  DROP FUNCTION rsum;
*/

#ifdef STANDARD
#include <stdio.h>
#include <string.h>
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

extern double rust_sum(double input[], size_t size);

my_bool summation_init( UDF_INIT* initid, UDF_ARGS* args, char* message );
void summation_deinit( UDF_INIT* initid );
void summation_reset( UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error );
void summation_add( UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error );
double summation( UDF_INIT* initid, UDF_ARGS* args, char* is_null, char *error );

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
    strcpy(message,"wrong number of arguments: rsum() requires one argument");
    return 1;
  }

  if (args->arg_type[0]!=REAL_RESULT && args->arg_type[0]!=INT_RESULT && args->arg_type[0] != DECIMAL_RESULT)
  {
    if (args->arg_type[0] == STRING_RESULT)
      strcpy(message,"rsum() requires a real or integer as parameter 1, received STRING");
    else
      strcpy(message,"rsum() requires a decimal, real or integer as parameter 1");
    return 1;
  }

  initid->decimals = NOT_FIXED_DEC;
  initid->maybe_null = 1;

  summation_data *buffer = malloc(sizeof(summation_data));
  buffer->count = 0;
  buffer->abscount=0;
  buffer->pages = 1;
  buffer->values = NULL;

  initid->maybe_null	= 1;
  initid->max_length	= 32;
  initid->ptr = (char*)buffer;

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



void summation_reset( UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* is_error )
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

  summation_add( initid, args, is_null, is_error );
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
    buffer->values[buffer->abscount++] = *((double*)args->args[0]);
    buffer->count++;
  }
}


double summation( UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* is_error )
{
  summation_data* buffer = (summation_data*)initid->ptr;

  if (buffer->abscount==0 || *is_error!=0)
  {
    *is_null = 1;
    return 0.0;
  }

  *is_null=0;
  if (buffer->abscount==1)
  {
    return buffer->values[0];
  }
  return rust_sum(buffer->values,buffer->abscount);
}

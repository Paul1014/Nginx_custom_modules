/**
 * @file   ngx_http_hello_world_module.c
 * @author António P. P. Almeida <appa@perusio.net>
 * @date   Wed Aug 17 12:06:52 2011
 *
 * @brief  A hello world module for Nginx.
 *
 * @section LICENSE
 *
 * Copyright (C) 2011 by Dominic Fallows, António P. P. Almeida <appa@perusio.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <string.h>
#include <stdlib.h>


#define MAX_LINE_SIZE 256
#define HELLO_WORLD "hello world\r\n"

static char *ngx_http_hello_world(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_hello_world_handler(ngx_http_request_t *r);



/**
 *  Get Cpu Idle per code
 * **/

typedef struct {
    char cpu_label[10];
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    double cpu_Idle_per;
    double cpu_Used_per;
    // Add more fields as needed
} CPUStats;

void parseCpuStats(char *line, CPUStats *stats) {
    sscanf(line, "%s %lu %lu %lu %lu", stats->cpu_label, &stats->user, &stats->nice, &stats->system, &stats->idle);
    stats->cpu_Idle_per = (double)stats->idle/(stats->user+stats->nice+stats->system+stats->idle);
    stats->cpu_Used_per = 1- stats->cpu_Idle_per;
}

int GetCpuIdlePer() {
    FILE *file;
    char line[MAX_LINE_SIZE];
    
    CPUStats cpuStats;

    // Open the /proc/stat file in read mode
    file = fopen("/proc/stat", "r");

    // Check if the file was successfully opened
    if (file == NULL) {
        perror("Could not open /proc/stat");
        return -1; // Return an error code
    }

    // Read the first line from the file
    if (fgets(line, sizeof(line), file) == NULL) {
        perror("Error reading from /proc/stat");
        fclose(file);
        return -1;
    }
    parseCpuStats(line, &cpuStats);
    fclose(file);

    return cpuStats.cpu_Used_per;
}


/**
* Get Meminfo 
*/
typedef struct {
    unsigned long total_memory_kb;
    unsigned long avail_memory_kb;
    double avail_memory_per;
} Memstat;

double GetMemUsedPer() {
    FILE *file;
    char buffer[256];
    Memstat *mem_stat = malloc(sizeof(Memstat)); // Allocate memory for mem_stat
    // Open /proc/meminfo for reading
    file = fopen("/proc/meminfo", "r");
    if (file == NULL) {
        perror("Error opening /proc/meminfo");
        return 1;
    }

    // Read and parse the content
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        // Check if the line contains "MemTotal"
        if (strstr(buffer, "MemTotal") != NULL) {
            // Extract the value from the line
            sscanf(buffer, "MemTotal: %lu kB", &mem_stat->total_memory_kb);
            
        }
        if (strstr(buffer, "MemAvailable") != NULL) {
            // Extract the value from the line
            sscanf(buffer, "MemAvailable: %lu kB", &mem_stat->avail_memory_kb);
            
        }
        if (mem_stat->avail_memory_kb > 0 && mem_stat->avail_memory_kb > 0) {
            break;
        }
    }

    // Close the file
    fclose(file);

    // Get Mem avail percent
    mem_stat->avail_memory_per = ((double)(mem_stat->total_memory_kb - mem_stat->avail_memory_kb) / mem_stat->total_memory_kb);
    double avail_memory_kb = mem_stat->avail_memory_per;
    // Print the total memory
    //printf("Total Memory: %lu kB\n", mem_stat->total_memory_kb);
    //printf("Available Memory: %lu kB\n", mem_stat->avail_memory_kb);
    //printf("Available Memory Percent: %.2f\n", mem_stat->avail_memory_per);
    free(mem_stat);
    return avail_memory_kb;
}



/**
 * This module provided directive: hello world.
 *
 */
static ngx_command_t ngx_http_hello_world_commands[] = {

    { ngx_string("hello_world"), /* directive */
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS, /* location context and takes
                                            no arguments*/
      ngx_http_hello_world, /* configuration setup function */
      0, /* No offset. Only one context is supported. */
      0, /* No offset when storing the module configuration on struct. */
      NULL},

    ngx_null_command /* command termination */
};

/* The hello world string. */
static u_char ngx_hello_world[] = HELLO_WORLD;

/* The module context. */
static ngx_http_module_t ngx_http_hello_world_module_ctx = {
    NULL, /* preconfiguration */
    NULL, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    NULL, /* create location configuration */
    NULL /* merge location configuration */
};

/* Module definition. */
ngx_module_t ngx_http_hello_world_module = {
    NGX_MODULE_V1,
    &ngx_http_hello_world_module_ctx, /* module context */
    ngx_http_hello_world_commands, /* module directives */
    NGX_HTTP_MODULE, /* module type */
    NULL, /* init master */
    NULL, /* init module */
    NULL, /* init process */
    NULL, /* init thread */
    NULL, /* exit thread */
    NULL, /* exit process */
    NULL, /* exit master */
    NGX_MODULE_V1_PADDING
};

/**
 * Content handler.
 *
 * @param r
 *   Pointer to the request structure. See http_request.h.
 * @return
 *   The status of the response generation.
 */
static ngx_int_t ngx_http_hello_world_handler(ngx_http_request_t *r)
{
    // u_char *ngx_hello_world = r->args.data;
    // size_t sz = r->args.len;

    ngx_table_elt_t  *h;
    ngx_buf_t *b;
    ngx_chain_t out;
    double cpu_Avail_per = GetCpuIdlePer();
    char cpu_Avail_per_str[6]; 
    sprintf(cpu_Avail_per_str, "%.3f", cpu_Avail_per);
    
    double mem_Used_per = GetMemUsedPer();
    char mem_Used_per_str[5]; 
    sprintf(mem_Used_per_str, "%.2f", mem_Used_per);

    /* Set the Content-Type header. */
    r->headers_out.content_type.len = sizeof("text/plain") - 1;
    r->headers_out.content_type.data = (u_char *) "text/plain";
    /* Sending the headers for the reply. */
    r->headers_out.status = NGX_HTTP_OK; /* 200 status code */
    /* Get the content length of the body. */
    
    // r->headers_out.content_length_n = sz;

    /* Add custom header  */
     h = ngx_list_push(&r->headers_out.headers);
    if (h == NULL) {
        return NGX_ERROR;
    }
    h->hash = 1;
    ngx_str_set(&h->key, "X-MemUtlization-Percent");
    ngx_str_set(&h->value, mem_Used_per_str);

    h = ngx_list_push(&r->headers_out.headers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    h->hash = 1;
    ngx_str_set(&h->key, "X-CpuUtlization-Percent");
    ngx_str_set(&h->value, cpu_Avail_per_str);
    

   



    /* Allocate a new buffer for sending out the reply. */
    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

    /* Insertion in the buffer chain. */
    out.buf = b;
    out.next = NULL; /* just one buffer */

    b->pos = ngx_hello_world; /* first position in memory of the data */
    b->last = ngx_hello_world + sizeof(ngx_hello_world) - 1; /* last position in memory of the data */
    b->memory = 1; /* content is in read-only memory */
    b->last_buf = 1; /* there will be no more buffers in the request */

    
    ngx_http_send_header(r); /* Send the headers */

    /* Send the body, and return the status code of the output filter chain. */
    return ngx_http_output_filter(r, &out);
} /* ngx_http_hello_world_handler */

/**
 * Configuration setup function that installs the content handler.
 *
 * @param cf
 *   Module configuration structure pointer.
 * @param cmd
 *   Module directives structure pointer.
 * @param conf
 *   Module configuration structure pointer.
 * @return string
 *   Status of the configuration setup.
 */
static char *ngx_http_hello_world(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf; /* pointer to core location configuration */

    /* Install the hello world handler. */
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_hello_world_handler;

    return NGX_CONF_OK;
} /* ngx_http_hello_world */
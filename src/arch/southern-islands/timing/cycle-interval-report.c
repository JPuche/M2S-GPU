/*
 *  Multi2Sim
 *  Copyright (C) 2012  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <lib/util/config.h>
#include <lib/util/debug.h>
#include <lib/util/file.h>
#include <lib/util/string.h>
//#include <lib/mhandle/mhandle.h>
#include <string.h>
#include <stdbool.h>

#include "cycle-interval-report.h"
#include "uop.h"

#include "compute-unit.h"
#include <lib/esim/esim.h>

#include <lib/util/estadisticas.h>
#include <mem-system/mshr.h>
#include <mem-system/mod-stack.h>

static int spatial_profiling_interval = 10000;
static int spatial_profiling_format = 0;
static char *si_spatial_report_section_name = "SISpatialReport";

//int si_spatial_report_active = 0;
int si_cu_spatial_report_active = 0;
int si_wf_spatial_report_active = 0;
int si_wg_spatial_report_active = 0;
int si_device_spatial_report_active = 0 ;

static char *cu_spatial_report_filename = "report-cu-spatial";
static char *device_spatial_report_filename = "report-device-spatial";
static char *wf_spatial_report_filename = "report-wavefront-spatial";
static char *wg_spatial_report_filename = "report-wavefront-spatial";

static FILE *cu_spatial_report_file;
//static FILE *device_mem_latency;
static FILE *device_spatial_report_file;
//static FILE *device_spatial_report_file_wg;
static FILE *wf_spatial_report_file;
static FILE *wg_spatial_report_file;
static FILE *stall_spatial_report_file;


void si_spatial_report_config_read(struct config_t *config)
{
	char *section;
	char *cu_file_name;
	char *device_file_name;
	char *wf_file_name;
	char *wg_file_name;

	char *aux_str; //PPP

	/*Nothing if section or config is not present */
	section = si_spatial_report_section_name;
	if (!config_section_exists(config, section))
	{
		/*no spatial profiling */
		return;
	}

	/* Spatial reports are active */
	//si_spatial_report_active = 1;

	/* output format */
	config_var_enforce(config, section, "Format");
	spatial_profiling_format = config_read_int(config, section,
		"Format", spatial_profiling_format);


	/* Interval */
	config_var_enforce(config, section, "Interval");
	spatial_profiling_interval = config_read_int(config, section,
		"Interval", spatial_profiling_interval);

	/* Compute Unit File name */
	//config_var_enforce(config, section, "cu_File");
	cu_file_name = config_read_string(config, section, "cu_file", NULL);
	if (cu_file_name && *cu_file_name)
	{
		si_cu_spatial_report_active = 1;
		//cu_spatial_report_filename = str_set(NULL, cu_file_name);
		//
		//aux_str = strdup(fran_extra_reports_path);
		aux_str = malloc(2000);
		strcpy(aux_str, fran_extra_reports_path);
		cu_spatial_report_filename = strcat(aux_str, cu_file_name);
		//aux_str = str_set(NULL, "");

		//printf("String %s\n", cu_spatial_report_filename);
		//printf("String %s\n", fran_extra_reports_path);

		cu_spatial_report_file = file_open_for_write(cu_spatial_report_filename);
		if (!cu_spatial_report_file)
			fatal("%s: could not open spatial report file",
					cu_spatial_report_filename);
	}
		//fatal("%s: %s: invalid or missing value for 'cu_File'",
		//	si_spatial_report_section_name, section);

	/* Device File name */
	device_file_name = config_read_string(config, section, "device_file", NULL);
	if (device_file_name && *device_file_name)
	{
		si_device_spatial_report_active = 1;

		//device_spatial_report_filename = str_set(NULL, device_file_name);
		//aux_str = strdup(fran_extra_reports_path);
		strcpy(aux_str, fran_extra_reports_path);
		device_spatial_report_filename = strcat(aux_str, device_file_name);
		//str_free(aux_str);
		//aux_str = str_set(NULL, "");

		//printf("String %s\n", device_spatial_report_filename);
		device_spatial_report_file = file_open_for_write(device_spatial_report_filename);
		//device_spatial_report_file_wg = file_open_for_write(str_concat(device_spatial_report_filename,"_wg"));
		//device_spatial_report_file_wf = file_open_for_write(str_concat(device_spatial_report_filename,"_wf"));
		if (!device_spatial_report_file)
			fatal("%s: could not open spatial report file", device_spatial_report_filename);
	}

	wg_file_name = config_read_string(config, section, "wg_file", NULL);
	if (wg_file_name && *wg_file_name)
	{
		si_wg_spatial_report_active = 1;

		wg_spatial_report_filename = str_set(NULL, wg_file_name);
		wg_spatial_report_file = file_open_for_write(wg_spatial_report_filename);

		if (!wg_spatial_report_file)
			fatal("%s: could not open spatial report file", wg_spatial_report_filename);
	}

	wf_file_name = config_read_string(config, section, "wf_file", NULL);
	if (wf_file_name && *wf_file_name)
	{
		si_wf_spatial_report_active = 1;

		wf_spatial_report_filename = str_set(NULL, wf_file_name);
		wf_spatial_report_file = file_open_for_write(wf_spatial_report_filename);

		if (!wf_spatial_report_file)
			fatal("%s: could not open spatial report file", wf_spatial_report_filename);
	}

	//stall_spatial_report_file = file_open_for_write("./stall");
	//fprintf(stall_spatial_report_file, "id, stalls, cycle, esim_time\n");

	if(!si_device_spatial_report_active && !si_cu_spatial_report_active)
		fatal("%s: %s: invalid or missing value for 'device_File' and %s: %s: invalid or missing value for 'cu_File'",
			device_file_name, section,cu_file_name, section);
}

void si_spatial_report_init()
{
	if(si_device_spatial_report_active)
		si_device_spatial_report_init();

	if(si_cu_spatial_report_active)
		si_cu_spatial_report_init();

	if(si_wf_spatial_report_active)
		si_wf_spatial_report_init();
}

void si_cu_spatial_report_init()
{
    fprintf(cu_spatial_report_file, "CU_id,total_i,simd_i,simd_op,scalar_i,v_mem_i,v_mem_op,s_mem_i,lds_i,lds_op,branch_i,");
    fprintf(cu_spatial_report_file, "mshr_wavefront_inflight,mappedWG,unmappedWG,WorkGroup_count,cycles,esim_time\n");
}

void si_wf_spatial_report_init()
{
	fprintf(wf_spatial_report_file,"wf_id,opc,inst_stall,mem_accesses_inflight,mem_unknown,mem_misses,mem_coalesce,exec_cycles,esim_time,dist_misses_0,dist_misses_1-5,dist_misses_6-10,dist_misses_11-15,dist_misses_16-20,dist_misses_21\n");
}

void si_device_spatial_report_init(SIGpu *device)
{
	//fprintf(device_spatial_report_file_wg,"op_counter,interval_cycles,esim_time\n");

	device->interval_statistics = calloc(1, sizeof(struct si_gpu_unit_stats));

        fprintf(device_spatial_report_file, "blocks_life_cycles_lvl0,blocks_life_count_lvl0,blocks_life_cycles_lvl1,blocks_life_count_lvl1,blocks_life_cycles_lvl2,blocks_life_count_lvl2,blocks_life_cycles_lvl3,blocks_life_count_lvl3,blocks_life_cycles_lvl4,blocks_life_count_lvl4,");
	fprintf(device_spatial_report_file, "mshr_wavefront_inflight,wait_for_mem_time,wait_for_mem_counter,gpu_idle,predicted_opc_op,predicted_opc_cyckes,MSHR_size,");
	fprintf(device_spatial_report_file, "accesses_L1_to_L2,accesses_L2_to_MM,accesses_L1_to_L2_hit,accesses_L1_to_L2_hit_count,accesses_L1_to_L2_miss,accesses_L1_to_L2_miss_count,accesses_L1_to_L2_retries,accesses_L1_to_L2_retries_count,dist0_99,dist100_199,dist200_299,dist300_399,dist400_499,dist500_599,dist600_9999,mem_acc_start,mem_acc_end,mem_acc_lat,load_start,load_end,load_lat,uop_load_end,uop_load_lat,uop_load_vmb_lat,uop_load_mm_lat,write_start,write_end,write_lat,");
	fprintf(device_spatial_report_file, "vcache_load_start,vcache_load_finish,scache_start,scache_finish,vcache_write_start,vcache_write_finish,cache_retry_lat,cache_retry_cont,");
	fprintf(device_spatial_report_file, "active_wavefronts,wavefronts_waiting_mem,");
	fprintf(device_spatial_report_file, "total_i,simd_i,simd_op,scalar_i,v_mem_i,v_mem_op,s_mem_i,lds_i,lds_op,branch_i,");
	fprintf(device_spatial_report_file, "mappedWG,unmappedWG,cycles,esim_time\n");
        
        //device_mem_latency = file_open_for_write("mem_latency");
        //fprintf(device_mem_latency, "accesos,latencia,cycle,esim_time\n");
}

void si_add_block_life_cycles(int level, long long cycles)
{
    assert(cycles > 0);
    si_gpu->blocks_life_cycles[level] += cycles;
    si_gpu->blocks_life_count[level]++;
}    

void si_spatial_report_done()
{
	if (si_cu_spatial_report_active){
		fclose(cu_spatial_report_file);
		cu_spatial_report_file = NULL;
		str_free(cu_spatial_report_filename);
	}

	if (si_device_spatial_report_active)
	{
		fclose(device_spatial_report_file);
		device_spatial_report_file = NULL;
		str_free(device_spatial_report_filename);
	}

	if (si_wg_spatial_report_active)
	{
		fclose(wg_spatial_report_file);
		wg_spatial_report_file = NULL;
		str_free(wg_spatial_report_filename);
	}

	if (si_wf_spatial_report_active)
	{
		fclose(wf_spatial_report_file);
		wf_spatial_report_file = NULL;
		str_free(wf_spatial_report_filename);
	}
}

void si_stalls_spatial_report(struct si_wavefront_t * wf)
{
	fprintf(stall_spatial_report_file, "%d, %lld, %lld, %lld\n",wf->id, wf->statistics->mem_misses - wf->statistics->prev_mem_misses, asTiming(wf->work_group->wavefront_pool->compute_unit->compute_device)->cycle, esim_time);
	fflush(stall_spatial_report_file);
}


void add_wait_for_mem_latency(struct si_compute_unit_t *compute_unit, long long cycles)
{
	compute_unit->compute_device->interval_statistics->wait_for_mem_time += cycles;
	compute_unit->compute_device->interval_statistics->wait_for_mem_counter++;
}

void si_cu_spatial_report_dump(struct si_compute_unit_t *compute_unit)
{
	if(!si_cu_spatial_report_active)
		return;

	FILE *f = cu_spatial_report_file;

	if(spatial_profiling_format == 0){
		fprintf(f,
			"CU,%d,MemAcc,%lld,MappedWGs,%lld,UnmappedWGs,%lld,ALUIssued,%lld,LDSIssued,%lld,Cycles,%lld\n",
			compute_unit->id,
			compute_unit->vector_mem_unit.inflight_mem_accesses,
			compute_unit->interval_mapped_work_groups,
			compute_unit->interval_unmapped_work_groups,
			compute_unit->interval_alu_issued,
			compute_unit->interval_lds_issued,
			compute_unit->interval_cycle);
	}else if(spatial_profiling_format == 1){
	fprintf(f,"%d,",compute_unit->id);
            
        //instruction report total_i, simd_i, scalar_i, v_mem_i, s_mem_i, lds_i
	fprintf(f, "%lld,", compute_unit->interval_statistics->instructions_counter);
	fprintf(f, "%lld,", compute_unit->interval_statistics->macroinst[simd_u]);
	fprintf(f, "%lld,", compute_unit->interval_statistics->op_counter[simd_u]);
	fprintf(f, "%lld,", compute_unit->interval_statistics->macroinst[scalar_u]);
	fprintf(f, "%lld,", compute_unit->interval_statistics->macroinst[v_mem_u]);
	fprintf(f, "%lld,", compute_unit->interval_statistics->op_counter[v_mem_u]);
	fprintf(f, "%lld,", compute_unit->interval_statistics->macroinst[s_mem_u]);
	fprintf(f, "%lld,", compute_unit->interval_statistics->macroinst[lds_u]);
	fprintf(f, "%lld,", compute_unit->interval_statistics->op_counter[lds_u]);
	fprintf(f, "%lld,", compute_unit->interval_statistics->macroinst[branch_u]);
                
        fprintf(f,"%lld,%lld,%lld,%d,%lld,%lld\n",
            compute_unit->interval_statistics->wavefronts_inflight,
            compute_unit->interval_statistics->interval_mapped_work_groups,
            compute_unit->interval_statistics->interval_unmapped_work_groups,
            compute_unit->work_group_count,
            compute_unit->interval_statistics->interval_cycles,
            esim_time);
        //free(compute_unit->interval_statistics);
        memset(compute_unit->interval_statistics, 0, sizeof(struct si_gpu_unit_stats));
        }

}

void si_vector_memory_report_new_inst(struct si_compute_unit_t *compute_unit, struct si_uop_t *uop)
{
        compute_unit->interval_statistics->macroinst[v_mem_u]++;
        compute_unit->interval_statistics->instructions_counter++;
	compute_unit->interval_statistics->op_counter[v_mem_u] += si_wavefront_count_active_work_items(uop->wavefront);
    
	compute_unit->compute_device->interval_statistics->macroinst[v_mem_u]++;
	compute_unit->compute_device->interval_statistics->instructions_counter++;
	compute_unit->compute_device->interval_statistics->op_counter[v_mem_u] += si_wavefront_count_active_work_items(uop->wavefront);

	uop->wavefront->work_group->op_counter += si_wavefront_count_active_work_items(uop->wavefront);
	uop->wavefront->statistics->op_counter[v_mem_u] += si_wavefront_count_active_work_items(uop->wavefront);
}

void si_branch_report_new_inst(struct si_compute_unit_t *compute_unit, struct si_uop_t *uop)
{
	compute_unit->interval_statistics->macroinst[branch_u]++;
        compute_unit->interval_statistics->instructions_counter++;
	compute_unit->interval_statistics->op_counter[branch_u]++;
        
        compute_unit->compute_device->interval_statistics->macroinst[branch_u]++;
	compute_unit->compute_device->interval_statistics->instructions_counter++;
	compute_unit->compute_device->interval_statistics->op_counter[branch_u]++;

	uop->wavefront->work_group->op_counter++;
	uop->wavefront->statistics->op_counter[branch_u]++;
}

void si_lds_report_new_inst(struct si_compute_unit_t *compute_unit, struct si_uop_t *uop)
{
	compute_unit->interval_statistics->macroinst[lds_u]++;
        compute_unit->interval_statistics->instructions_counter++;
    
        compute_unit->interval_lds_issued++;
	compute_unit->compute_device->interval_statistics->macroinst[lds_u]++;
	compute_unit->compute_device->interval_statistics->instructions_counter++;

	compute_unit->compute_device->interval_statistics->op_counter[
	lds_u] += si_wavefront_count_active_work_items(uop->wavefront);

	uop->wavefront->work_group->op_counter += si_wavefront_count_active_work_items(uop->wavefront);
	uop->wavefront->statistics->op_counter[lds_u] += si_wavefront_count_active_work_items(uop->wavefront);
}


void si_scalar_alu_report_new_inst(struct si_compute_unit_t *compute_unit, struct si_uop_t *uop)
{
        compute_unit->interval_statistics->macroinst[scalar_u]++;
        compute_unit->interval_statistics->instructions_counter++;
	compute_unit->interval_statistics->op_counter[scalar_u]++;
    
	compute_unit->interval_alu_issued++;
	compute_unit->compute_device->interval_statistics->macroinst[scalar_u]++;
	compute_unit->compute_device->interval_statistics->instructions_counter++;
	compute_unit->compute_device->interval_statistics->op_counter[scalar_u]++;

	uop->wavefront->work_group->op_counter++;
	uop->wavefront->statistics->op_counter[scalar_u]++;
}

void si_simd_alu_report_new_inst(struct si_compute_unit_t *compute_unit, struct si_uop_t *uop)
{
        compute_unit->interval_statistics->macroinst[simd_u]++;
        compute_unit->interval_statistics->instructions_counter++;
	compute_unit->interval_statistics->op_counter[simd_u] += si_wavefront_count_active_work_items(uop->wavefront);
    
        compute_unit->compute_device->interval_statistics->macroinst[simd_u]++;
	compute_unit->compute_device->interval_statistics->instructions_counter++;

	compute_unit->compute_device->interval_statistics->op_counter[simd_u] += si_wavefront_count_active_work_items(uop->wavefront);

	uop->wavefront->work_group->op_counter += si_wavefront_count_active_work_items(uop->wavefront);
	uop->wavefront->statistics->op_counter[simd_u] += si_wavefront_count_active_work_items(uop->wavefront);
}

void si_report_global_mem_inflight( struct si_compute_unit_t *compute_unit, struct si_uop_t *uop)
{
	/* Read stage adds a negative number for accesses added
	 * Write stage adds a positive number for accesses finished
	 */
	compute_unit->vector_mem_unit.inflight_mem_accesses += uop->active_work_items;
	compute_unit->compute_device->interval_statistics->memory.accesses_started += uop->active_work_items;

	//latency
	if(uop->vector_mem_read)
	{
		compute_unit->compute_device->interval_statistics->memory.load_start += uop->active_work_items;
	}
	if(uop->vector_mem_write)
	{
		compute_unit->compute_device->interval_statistics->memory.write_start += uop->active_work_items;
	}


}

void si_report_global_mem_finish( struct si_compute_unit_t *compute_unit, struct si_uop_t *uop)
{
	/* Read stage adds a negative number for accesses added */
	/* Write stage adds a positive number for accesses finished */
	compute_unit->vector_mem_unit.inflight_mem_accesses -= uop->active_work_items;
	compute_unit->compute_device->interval_statistics->memory.accesses_finished += uop->active_work_items;
	compute_unit->compute_device->interval_statistics->memory.accesses_latency += (asTiming(si_gpu)->cycle - uop->send_cycle) * uop->active_work_items;
	//latency
	if(uop->vector_mem_read)
	{
		compute_unit->compute_device->interval_statistics->memory.load_finish += uop->active_work_items;
		compute_unit->compute_device->interval_statistics->memory.load_latency += (asTiming(si_gpu)->cycle - uop->send_cycle) * uop->active_work_items;
		compute_unit->compute_device->interval_statistics->memory.uop_load_finish++;
		compute_unit->compute_device->interval_statistics->memory.uop_load_latency += asTiming(si_gpu)->cycle - uop->send_cycle;
		compute_unit->compute_device->interval_statistics->memory.uop_load_vmb_latency += asTiming(si_gpu)->cycle - uop->mem_access_finish_cycle;
		if(uop->mem_mm_accesses)
			compute_unit->compute_device->interval_statistics->memory.uop_load_mm_latency += uop->mem_mm_latency / uop->mem_mm_accesses;
	}
	if(uop->vector_mem_write)
	{
		compute_unit->compute_device->interval_statistics->memory.write_finish += uop->active_work_items;
		compute_unit->compute_device->interval_statistics->memory.write_latency += (asTiming(si_gpu)->cycle - uop->send_cycle) * uop->active_work_items;
	}
        
        long long lat = asTiming(si_gpu)->cycle - uop->send_cycle;
        if(lat < 100){
            compute_unit->compute_device->interval_statistics->memory.dist0_99++;  
        }else if(lat < 200){
            compute_unit->compute_device->interval_statistics->memory.dist100_199++; 
        }else if(lat < 300){
            compute_unit->compute_device->interval_statistics->memory.dist200_299++;
        }else if(lat < 400){
            compute_unit->compute_device->interval_statistics->memory.dist300_399++;
        }else if(lat < 500){
            compute_unit->compute_device->interval_statistics->memory.dist400_499++;
        }else if(lat < 600){
            compute_unit->compute_device->interval_statistics->memory.dist500_599++;
        }else{
            compute_unit->compute_device->interval_statistics->memory.dist600_9999++;
        }
        

        //device_mem_latency = file_open_for_write("mem_latency");
        //fprintf(device_mem_latency,"%lld,%lld,%lld,%lld\n", uop->active_work_items,(asTiming(si_gpu)->cycle - uop->send_cycle), asTiming(si_gpu)->cycle, esim_time);
        //fflush(device_mem_latency);
        

}

void si_report_gpu_idle(SIGpu *device)
{
	if(si_device_spatial_report_active)
		device->interval_statistics->gpu_idle = 1;
}

void si_report_mapped_work_group(struct si_compute_unit_t *compute_unit)
{
	if(si_device_spatial_report_active)
	{
		/*TODO Add calculation here to change this to wavefront pool entries used */
		compute_unit->interval_mapped_work_groups++;
		compute_unit->compute_device->interval_statistics->interval_mapped_work_groups++;
                compute_unit->interval_statistics->interval_mapped_work_groups++;
	}
}


void si_report_unmapped_work_group(struct si_compute_unit_t *compute_unit)
{
	/*TODO Add calculation here to change this to wavefront pool entries used */
	compute_unit->interval_unmapped_work_groups++;
	compute_unit->compute_device->interval_statistics->interval_unmapped_work_groups++;
        compute_unit->interval_statistics->interval_unmapped_work_groups++;
}


void si_cu_interval_update(struct si_compute_unit_t *compute_unit)
{
	/* If interval - reset the counters in all the engines */
	compute_unit->interval_cycle++;
        compute_unit->interval_statistics->interval_cycles++;

	if (si_cu_spatial_report_active && !(asTiming(si_gpu)->cycle % spatial_profiling_interval))
	{
		si_cu_spatial_report_dump(compute_unit);

		/*
		 * This counter is not reset since memory accesses could still
		 * be in flight in the hierarchy
		 * compute_unit->inflight_mem_accesses = 0;
		 */
		compute_unit->interval_cycle = 0;
		compute_unit->interval_mapped_work_groups = 0;
		compute_unit->interval_unmapped_work_groups = 0;
		compute_unit->interval_alu_issued = 0;
		compute_unit->interval_lds_issued = 0;
	}
}

//int contador_mshr = 20;

void si_device_interval_update(SIGpu *device)
{
	/* If interval - reset the counters in all the engines */

	if (si_device_spatial_report_active)
	{
		device->interval_statistics->interval_cycles++;
                
                for(int j = 0; j < si_gpu_num_compute_units; j++)
                {
                        device->accesses_L1_to_l2 += device->compute_units[j]->accesses_L1_to_L2;
                        device->accesses_L2_to_MM += device->compute_units[j]->accesses_L2_to_MM;
                }
                

		if(device->interval_statistics->interval_cycles >= spatial_profiling_interval)
		{
			si_device_spatial_report_dump(device);
			device->op = device->interval_statistics->macroinst[scalar_u] + device->interval_statistics->op_counter[simd_u] + device->interval_statistics->op_counter[v_mem_u] + device->interval_statistics->macroinst[s_mem_u] + device->interval_statistics->op_counter[lds_u];
			device->cycles = device->interval_statistics->interval_cycles;

			/*
			 * This counter is not reset since memory accesses could still
			 * be in flight in the hierarchy
			 * compute_unit->inflight_mem_accesses = 0;
			 */
			/*device->interval_cycle = 0;
			device->interval_mapped_work_groups = 0;
			compute_unit->interval_unmapped_work_groups = 0;
			compute_unit->interval_alu_issued = 0;
			compute_unit->interval_lds_issued = 0;*/
			/*contador_mshr--;

			if(flag_mshr_dynamic_enabled && contador_mshr == 0)
			{
			  mshr_control2();
				contador_mshr=20;
			}*/

			memset(device->interval_statistics, 0, sizeof(struct si_gpu_unit_stats));
			//device->interval_statistics = calloc(1, sizeof(struct si_gpu_unit_stats));
		}
	}
}

void si_device_interval_update_force(SIGpu *device)
{
	if(si_device_spatial_report_active)
	{
		if(device->interval_statistics->interval_cycles > 0)
		{
			si_device_spatial_report_dump(device);
			memset(device->interval_statistics, 0, sizeof(struct si_gpu_unit_stats));
			//device->interval_statistics = calloc(1, sizeof(struct si_gpu_unit_stats));
		}
	}
}

void analizar_wavefront(SIGpu *device)
{
	int i,j,w;
	struct si_compute_unit_t *compute_unit;
	struct si_wavefront_t *wavefront;

	for(j = 0; j < si_gpu_num_compute_units; j++)
	{
		compute_unit = device->compute_units[j];
		for(w = 0; w < si_gpu_num_wavefront_pools; w++)
		{
			for (i = 0; i < si_gpu_max_wavefronts_per_wavefront_pool; i++)
			{
				wavefront = compute_unit->wavefront_pools[w]->entries[i]->wavefront;

				/* No wavefront */
				if (!wavefront)
					continue;

				device->interval_statistics->active_wavefronts++;

				/* If the wavefront finishes, there still may be outstanding
				 * memory operations, so if the entry is marked finished
				 * the wavefront must also be finished, but not vice-versa */
				if (wavefront->wavefront_pool_entry->wavefront_finished)
				{
					continue;
				}

				/* Wavefront is finished but other wavefronts from workgroup
				 * remain.  There may still be outstanding memory operations,
				 * but no more instructions should be fetched. */
				if (wavefront->finished)
					continue;

				/* Wavefront is ready but waiting at barrier */
				if (wavefront->wavefront_pool_entry->wait_for_barrier)
				{
					continue;
				}

				/* Wavefront is ready but waiting on outstanding
				 * memory instructions */
				if (wavefront->wavefront_pool_entry->wait_for_mem)
				{
					if (wavefront->wavefront_pool_entry->lgkm_cnt ||
						wavefront->wavefront_pool_entry->vm_cnt)
					{
						device->interval_statistics->wavefronts_waiting_mem++;
					}
				}
			}
		}

		struct list_t *wavefront_list = list_create();
		for(int h = 0;h < list_count(compute_unit->vector_cache->mshr->access_list);h++)
		{
			struct mod_stack_t *stack = list_get(compute_unit->vector_cache->mshr->access_list,h);

			if(list_index_of(wavefront_list, stack->wavefront) != -1)
			{
				continue;
			}

			list_add(wavefront_list,stack->wavefront);

			bool sumar = true;

			for (int i = 0; i < list_count(compute_unit->vector_cache->mshr->waiting_list); i++)
			{
				struct mod_stack_t *waiting_stack = list_get(compute_unit->vector_cache->mshr->waiting_list,i);
				if(stack->wavefront == waiting_stack->wavefront)
				{
					sumar = false;
					break;
				}
			}
			if(sumar)
			{
				device->interval_statistics->wavefronts_inflight++;
                                compute_unit->interval_statistics->wavefronts_inflight++;
			}
		}
	}
}

void si_device_spatial_report_dump(SIGpu *device)
{
	if(!si_device_spatial_report_active)
		return;

	FILE *f = device_spatial_report_file;
        
        for(int i = 0; i<5;i++){
            fprintf(f, "%lld,",si_gpu->blocks_life_cycles[i]);
            fprintf(f, "%lld,",si_gpu->blocks_life_count[i]);
        }

	analizar_wavefront(device);
	fprintf(f, "%lld,", device->interval_statistics->wavefronts_inflight);
	fprintf(f, "%lld,", device->interval_statistics->wait_for_mem_time);
	fprintf(f, "%lld,", device->interval_statistics->wait_for_mem_counter);

	fprintf(f, "%lld,", device->interval_statistics->gpu_idle);

	fprintf(f, "%lld,", device->interval_statistics->predicted_opc_op);
	fprintf(f, "%lld,", device->interval_statistics->predicted_opc_cycles);

        if(device->compute_units[0]->vector_cache->mshr->testing)
            fprintf(f,"nan,");
        else
            fprintf(f, "%d,", device->compute_units[0]->vector_cache->mshr->size);

        fprintf(f, "%lld,", device->accesses_L1_to_l2);
        device->accesses_L1_to_l2 = 0;
        fprintf(f, "%lld,", device->accesses_L2_to_MM);
        device->accesses_L2_to_MM = 0;
        fprintf(f, "%lld,", device->accesses_L1_to_l2_hit);
        device->accesses_L1_to_l2_hit = 0;
        fprintf(f, "%lld,", device->accesses_L1_to_l2_hit_count);
        device->accesses_L1_to_l2_hit_count = 0;
        fprintf(f, "%lld,", device->accesses_L1_to_l2_miss);
        device->accesses_L1_to_l2_miss = 0;
        fprintf(f, "%lld,", device->accesses_L1_to_l2_miss_count);
        device->accesses_L1_to_l2_miss_count = 0;
        fprintf(f, "%lld,", device->accesses_L1_to_l2_retries);
        device->accesses_L1_to_l2_retries = 0;
        fprintf(f, "%lld,", device->accesses_L1_to_l2_retries_count);
        device->accesses_L1_to_l2_retries_count = 0;
        
        fprintf(f, "%lld,", device->interval_statistics->memory.dist0_99);
        fprintf(f, "%lld,", device->interval_statistics->memory.dist100_199);
        fprintf(f, "%lld,", device->interval_statistics->memory.dist200_299);
        fprintf(f, "%lld,", device->interval_statistics->memory.dist300_399);
        fprintf(f, "%lld,", device->interval_statistics->memory.dist400_499);
        fprintf(f, "%lld,", device->interval_statistics->memory.dist500_599);
        fprintf(f, "%lld,", device->interval_statistics->memory.dist600_9999);
        
	fprintf(f, "%lld,", device->interval_statistics->memory.accesses_started);
	fprintf(f, "%lld,", device->interval_statistics->memory.accesses_finished);
	fprintf(f, "%lld,", device->interval_statistics->memory.accesses_latency);
	fprintf(f, "%lld,", device->interval_statistics->memory.load_start);
	fprintf(f, "%lld,", device->interval_statistics->memory.load_finish);
	fprintf(f, "%lld,", device->interval_statistics->memory.load_latency);
	fprintf(f, "%lld,", device->interval_statistics->memory.uop_load_finish);
	fprintf(f, "%lld,", device->interval_statistics->memory.uop_load_latency);
	fprintf(f, "%lld,", device->interval_statistics->memory.uop_load_vmb_latency);
	fprintf(f, "%lld,", device->interval_statistics->memory.uop_load_mm_latency);
	fprintf(f, "%lld,", device->interval_statistics->memory.write_start);
	fprintf(f, "%lld,", device->interval_statistics->memory.write_finish);
	fprintf(f, "%lld,", device->interval_statistics->memory.write_latency);


	fprintf(f, "%lld,", device->interval_statistics->vcache_load_start);
	fprintf(f, "%lld,", device->interval_statistics->vcache_load_finish);
	fprintf(f, "%lld,", device->interval_statistics->scache_load_start);
	fprintf(f, "%lld,", device->interval_statistics->scache_load_finish);
	fprintf(f, "%lld,", device->interval_statistics->vcache_write_start);
	fprintf(f, "%lld,", device->interval_statistics->vcache_write_finish);
	fprintf(f, "%lld,", device->interval_statistics->cache_retry_lat);
	fprintf(f, "%lld,", device->interval_statistics->cache_retry_cont);

	fprintf(f, "%lld,", device->interval_statistics->active_wavefronts);
	fprintf(f, "%lld,", device->interval_statistics->wavefronts_waiting_mem);

	//instruction report total_i, simd_i, scalar_i, v_mem_i, s_mem_i, lds_i
	fprintf(f, "%lld,", device->interval_statistics->instructions_counter);
	fprintf(f, "%lld,", device->interval_statistics->macroinst[simd_u]);
	fprintf(f, "%lld,", device->interval_statistics->op_counter[simd_u]);
	fprintf(f, "%lld,", device->interval_statistics->macroinst[scalar_u]);
	fprintf(f, "%lld,", device->interval_statistics->macroinst[v_mem_u]);
	fprintf(f, "%lld,", device->interval_statistics->op_counter[v_mem_u]);
	fprintf(f, "%lld,", device->interval_statistics->macroinst[s_mem_u]);
	fprintf(f, "%lld,", device->interval_statistics->macroinst[lds_u]);
	fprintf(f, "%lld,", device->interval_statistics->op_counter[lds_u]);
	fprintf(f, "%lld,", device->interval_statistics->macroinst[branch_u]);

	fprintf(f, "%lld,", device->interval_statistics->interval_mapped_work_groups);
	fprintf(f, "%lld,", device->interval_statistics->interval_unmapped_work_groups);
  // fixme change spatial_profiling_interval for cycle_counter or device->interval_cycle
	fprintf(f,"%lld,%lld", device->interval_statistics->interval_cycles,	esim_time);

	fprintf(f,"\n");
	fflush(f);

}

void si_work_group_report_dump(struct si_work_group_t *wg)
{
	if(!si_wg_spatial_report_active)
		return;

	fprintf(wg_spatial_report_file,"%d,",wg->id);
	fprintf(wg_spatial_report_file,"%lld,",wg->op_counter);

	fprintf(wg_spatial_report_file,"%lld,%lld",
	asTiming(wg->wavefront_pool->compute_unit->compute_device)->cycle - wg->start_cycle, esim_time);
	fprintf(wg_spatial_report_file,"\n");
	//wg->wavefront_pool->compute_unit->compute_device->cycle_last_wg_op_counter_report = asTiming(wg->wavefront_pool->compute_unit->compute_device)->cycle;
	fflush(wg_spatial_report_file);
}

void si_wavefront_report_dump(struct si_wavefront_t *wavefront)
{
	if(!si_wf_spatial_report_active)
		return;

	fprintf(wf_spatial_report_file,"%d,",wavefront->id);
	long long total_op = wavefront->statistics->op_counter[simd_u] + wavefront->statistics->op_counter[scalar_u] + wavefront->statistics->op_counter[s_mem_u] + wavefront->statistics->op_counter[v_mem_u] + wavefront->statistics->op_counter[lds_u] + wavefront->statistics->op_counter[branch_u];
	fprintf(wf_spatial_report_file,"%lld,",total_op);

	fprintf(wf_spatial_report_file,"%lld,%lld,%lld,%lld,%lld,", wavefront->statistics->inst_stall, wavefront->statistics->mem_accesses_inflight, wavefront->statistics->mem_unknown, wavefront->statistics->mem_misses, wavefront->statistics->mem_coalesce);

	fprintf(wf_spatial_report_file,"%lld,%lld,", wavefront->finish_cycle - wavefront->work_group->start_cycle, esim_time);
	fprintf(wf_spatial_report_file,"%lld,%lld,%lld,%lld,%lld,%lld",wavefront->statistics->dist_misses_0,wavefront->statistics->dist_misses_1_5,wavefront->statistics->dist_misses_6_10,wavefront->statistics->dist_misses_11_15,wavefront->statistics->dist_misses_16_20,wavefront->statistics->dist_misses_21);
	fprintf(wf_spatial_report_file,"\n");
	fflush(wf_spatial_report_file);
	//wg->wavefront_pool->compute_unit->compute_device->cycle_last_wg_op_counter_report = asTiming(wg->wavefront_pool->compute_unit->compute_device)->cycle;
}

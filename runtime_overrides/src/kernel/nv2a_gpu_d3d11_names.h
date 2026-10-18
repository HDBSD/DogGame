/* Private symbol namespace for the retained reference backend. Include before
 * nv2a_gpu.h in its implementation; consumers use the public dispatch API. */
#define nv_gpu_enabled nv_d3d11_enabled
#define nv_gpu_begin nv_d3d11_begin
#define nv_gpu_triangle nv_d3d11_triangle
#define nv_gpu_append_vertex nv_d3d11_append_vertex
#define nv_gpu_indexed_triangle nv_d3d11_indexed_triangle
#define nv_gpu_indexed_upload_enabled nv_d3d11_indexed_upload_enabled
#define nv_gpu_end nv_d3d11_end
#define nv_gpu_sync nv_d3d11_sync
#define nv_gpu_sync_reason nv_d3d11_sync_reason
#define nv_gpu_sync_depth nv_d3d11_sync_depth
#define nv_gpu_prepare_depth_change nv_d3d11_prepare_depth_change
#define nv_gpu_discard_overwritten nv_d3d11_discard_overwritten
#define nv_gpu_clear_attachments nv_d3d11_clear_attachments
#define nv_gpu_present_image nv_d3d11_present_image
#define nv_gpu_report nv_d3d11_report
#define nv_gpu_texture_barrier nv_d3d11_texture_barrier
#define nv_gpu_shutdown nv_d3d11_shutdown

#include <scamp5.hpp>

using namespace SCAMP5_PE;
volatile int diffuse_iterations = 5;
volatile int diffuse_iterations1 = 1;
volatile int diffuse_iterations2 = 1;
volatile int threshold;

int main(){

	// Initialization
	vs_init(); //M0 core 处理器功能：初始化

	// Setup Host GUI
	vs_gui_set_info(VS_M0_PROJECT_INFO_STRING); //M0 core 处理器功能：gui 信息设置

	auto display_1 = vs_gui_add_display("half-scale", 0, 0);
	auto display_2 = vs_gui_add_display("sobel edge", 0, 1);
	//auto display_3 = vs_gui_add_display("laplacian edge", 1, 0);
	vs_gui_add_slider("threshold: ", 0, 255, 127, &threshold); //一个指向变量的指针，当滑块的值变化时，该变量将被自动更新。例如，&threshold


	while (1) {
		vs_frame_loop_control();

		// 
		scamp5_in(E, threshold);

		scamp5_kernel_begin();

		get_image(C);  // 从 [0, 255] 转换到 [127, 255],归一化到[0-127]

		diva(C, E, F);
		movx(A, C, north);
		movx(B, C, south);
		add(A, A, B);
		movx(D, C, east);
		movx(E, C, west);

		add(D, D, E); //west+east
		add(D, D, A); //west+east+north+south
		add(D, D, C);
		//add(D, D, C); //west+east+north+south+2*image
		//divq(B, D);
		//divq(A, B);
		//divq(B, A);  //B[128,255]-----normalized to [0,127]
		mov(E, D);
		scamp5_kernel_end();

		scamp5_diffuse(E, diffuse_iterations); //D diffuse滤波  

		scamp5_kernel_begin();

		sub(A, D, E);// Bioplor ON
		sub(F, E,D);  // Bioplor OFF
		add(A, A, C); // Bioplor ON Enhanced
		add(F, F, C); // Bioplor OFF Enhanced
		sub(E, A, F); // Parvo 
		divq(B, C);
		add(E, E, B);  //Parvo 边缘细节同时增强Edge details are enhanced simultaneously

		scamp5_kernel_end();

		scamp5_kernel_begin();
		res(D);
		res(B);
		res(C);
	
		// High-pass filter ON: current output = previous output + current input - previous input
		//E(t)= A(t)+A(t-1)+E(t-1)
		sub(C, A, B); // C = current input - previous input = current output
		add(C, C, D); // C = previous outputD + (current input - previous input)
		divq(E, C);
		
		// O(t) = max(O(t), 0)
		abs(F, C);
		sub(F,F,C);  //F = | C | -C
		where(F);    ////这些负值通常代表亮度的减少（例如，从明到暗的过渡）
		MOV(R8, FLAG);
		res(C);      // C = 0  (Set D for the negative region to 0)
		all();       // Reset FLAG to all pixels

		// Update previous input and output
		mov(B, A); // B   update previous input
		mov(D, C); // D   previous output (update previous output)
		scamp5_diffuse(D, diffuse_iterations1);
		divq(F, E);
		add(D, D, E); // enhance

		res(B);
		res(C);
		// High-pass filter ON: current output = previous output + current input - previous input
		sub(C, F, B); //  C = current input - previous input = current output
		add(C, C, E);  //  C = previous output E + (current input - previous input)
		divq(E, C); 
		
		// O(t) = max(O(t), 0)
		abs(A, C);
		sub(A, A, C); //A = | C | -C
		where(A);   
		MOV(R9, FLAG);
		res(C);      // C = 0  (Set D for the negative region to 0)
		all();       // Reset FLAG to all pixels

		mov(B, F);  //  B   update previous input
		mov(E, C);   // E   previous output (update previous output)
		scamp5_kernel_end();

		scamp5_diffuse(E, diffuse_iterations2); //LP

		scamp5_kernel_begin();


		get_image(C);
		divq(A, C);
		add(E, E, A); //enhance

		// MAGNO ouput 
		add(D, D, E);
		scamp5_kernel_end();

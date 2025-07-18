%铜活化中子产额分析
%主要输入：
%①符合计数文件
%②Cu62和Cu64的511gamma强度比例
%③计算中子产额的时间间隔，单位s
%输出：
clc;
clear;
close all;
%% 输入参数
% ratioCu62 = 0.997579432;
% ratioCu64 = 1 - ratioCu62;
ratioCu62 = 0.99;
ratioCu64 = 1 - ratioCu62;

deltaT = 60; %单位s
halflife_Cu62 = 9.67*60; %单位s，Cu62的半衰期
halflife_Cu64 = 12.7*60*60; % 单位s,Cu64的半衰期
timeWidth_tmp = 100;% 时间窗宽度,ns

%% 选择数据文件，读取数据
[filename, pathname, ~] = uigetfile({'*.txt';'*.符合计数'},'请选择一个符合计数文件');    
path = fullfile(pathname,filename);
opts = detectImportOptions(path);
opts.VariableTypes = {'double', 'double', 'double', 'double','double','double','double'};
opts.VariableNames = {'time', 'CountRate1', 'CountRate2', 'ConCount_single','ConCount_multiple','deathRatio1','deathRatio2'};
coincidence_data = readtable(path, opts);
count = table2array(coincidence_data);

%% 预处理
%单位转化
lamda62 = log(2) / halflife_Cu62;
lamda64 = log(2) / halflife_Cu64;
timeWidth_tmp = timeWidth_tmp *1.0 / 1e9; %转化为s

%分配数组
start_time = count(1,1);
end_time = count(end,1);
num = floor((end_time - start_time)/deltaT);
N1 = zeros(num,1);
N2 = N1;
Nc = N1;
death_ratio1 = N1; %死时间占比
death_ratio2 = N1; %死时间占比
Nco = N1;
N10 = N1;
N20 = N10;
Nco0 = N10;
factor = N10;
time = zeros(num,2);

%给出相对初始活度
for i = 1:num
    left = (i-1)*deltaT+1;
    right = i*deltaT;
%     deathRatio1 = 0.01*(count(left:right,6));
%     deathRatio2 = 0.01*(count(left:right,7));
    N1(i) = sum(count(left:right,2));
    N2(i) = sum(count(left:right,3));
    Nc(i) = sum(count(left:right,4));
    death_ratio1(i) = sum(count(left:right,6))*0.01 / deltaT;%求和后再给出平均每秒死时间占比
    death_ratio2(i) = sum(count(left:right,7))*0.01 / deltaT;
    
    %偶然符合修正
    Nco(i) = (Nc(i)*deltaT - 2*timeWidth_tmp*N1(i)*N2(i))/(deltaT - timeWidth_tmp * (N1(i) + N2(i)));
    
    %死时间修正
    N10(i) = N1(i) /(1 - death_ratio1(i));
    N20(i) = N2(i) /(1 - death_ratio2(i));
    Nco0(i) = Nco(i) / (1-death_ratio1(i))/(1-death_ratio2(i)); 
    
    left_time = count(left,1);
    right_time = count(right,1);
    time(i,1) = left_time;
    time(i,2) = right_time;
    factor(i) = ratioCu62/lamda62*(exp(-lamda62*(left_time-1)) - exp(-lamda62*right_time)) + ...
            ratioCu64/lamda64*(exp(-lamda64*(left_time-1)) - exp(-lamda64*right_time));
end
at_omiga = N10 .* N20 ./ Nco0 ./ factor;

%% 换算为中子产额

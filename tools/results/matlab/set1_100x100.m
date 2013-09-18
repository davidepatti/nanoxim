% fname: out_matlab/set1_100x100.m
% ../nanoxim -dimx 100 -dimy 100  -disr -cyclelinks 3 -bootstrap center -bootstrap_immunity -bootstrap_timeout 10000 

function [node_coverage, link_coverage, nsegments, avg_seg_length] = set1_100x100(symbol)

data = [
% defective_nodes  node_coverage  link_coverage      nsegments avg_seg_length
                0         0.9973       0.996212           3721        2.68019
                0         0.9973       0.996212           3721        2.68019
             0.05         0.9441       0.891465           2655        3.55593
             0.05         0.9563       0.915202           3088        3.09683
              0.1         0.8888       0.797677           2583        3.44096
              0.1         0.8895       0.799293           2567        3.46513
             0.15         0.8307       0.712576           2295        3.61961
             0.15         0.8222       0.697323           2311        3.55777
              0.2         0.7484       0.603384           1995        3.75138
              0.2         0.0022     0.00156566              5            4.4
             0.25         0.6268       0.485051           1537        4.07807
             0.25         0.6827       0.530354           1753        3.89447
              0.3          0.215        0.15904            569        3.77856
              0.3         0.2623       0.193081            626         4.1901
             0.35         0.0326      0.0235859             77        4.23377
             0.35         0.0444      0.0312121             86        5.16279
              0.4         0.0012    0.000757576              2              6
              0.4         0.0009    0.000606061              3              3
             0.45         0.0001              0              1              1
             0.45         0.0014    0.000959596              5            2.8
              0.5         0.0036     0.00267677             12              3
              0.5         0.0013    0.000858586              2            6.5
];

rows = size(data, 1);
cols = size(data, 2);

node_coverage = [];
for i = 1:rows/2,
   ifirst = (i - 1) * 2 + 1;
   ilast  = ifirst + 2 - 1;
   tmp = data(ifirst:ilast, cols-4+1);
   avg = mean(tmp);
   [h sig ci] = ttest(tmp, 0.1);
   ci = (ci(2)-ci(1))/2;
   node_coverage = [node_coverage; data(ifirst, 1:cols-4), avg ci];
end

figure(1);
hold on;
title('Node Coverage')
plot(node_coverage(:,1), node_coverage(:,2), symbol);
ylim([0 1])
xlabel('Node Defect Rate')
ylabel('node_coverage')
x = [0:0.05:0.5]
y = 1-x
plot(x,y,'--r')
legend('DiSR','Ideal')


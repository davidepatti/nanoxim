% fname: out_matlab/set3_0_10x10.m
% ../nanoxim -dimx 10 -dimy 10  -disr -cyclelinks 3 -bootstrap_immunity -bootstrap_timeout 100 

function [node_coverage, link_coverage, nsegments, avg_seg_length, latency] = set3_0_10x10(symbol)

data = [
%       bootstrap  node_coverage  link_coverage      nsegments avg_seg_length        latency
                0              1              1             39         2.5641           1390
                3              1              1             46        2.17391           1255
                6              1              1             51        1.96078           1147
                9              1       0.994444             47        2.12766           1292
               12              1              1             42        2.38095           1408
               15              1              1             47        2.12766           1256
               18              1              1             56        1.78571           1245
               21              1              1             35        2.85714           1379
               24              1              1             49        2.04082           1261
               27              1              1             51        1.96078           1147
               30              1              1             45        2.22222           1407
               33              1       0.994444             40            2.5           1354
               36              1              1             51        1.96078           1243
               39              1              1             37         2.7027           1188
               42              1              1             38        2.63158           1389
               45              1              1             41        2.43902           1273
               48              1              1             32          3.125           1200
               51              1              1             31        3.22581           1365
               54              1              1             34        2.94118           1380
               57              1              1             37         2.7027           1325
               60              1              1             42        2.38095           1470
               63              1              1             30        3.33333           1462
               66              1              1             40            2.5           1317
               69              1              1             37         2.7027           1438
               72              1              1             31        3.22581           1491
               75              1              1             39         2.5641           1332
               78              1              1             37         2.7027           1291
               81           0.08      0.0555556              2              4           1311
               84              1              1             40            2.5           1389
               87              1              1             34        2.94118           1423
               90              1              1             36        2.77778           1727
               93              1              1             22        4.54545           1533
               96              1              1             33         3.0303           1377
               99           0.42       0.372222             12            3.5           1226
];

rows = size(data, 1);
cols = size(data, 2);

node_coverage = [];
for i = 1:rows/1,
   ifirst = (i - 1) * 1 + 1;
   ilast  = ifirst + 1 - 1;
   tmp = data(ifirst:ilast, cols-5+5);
   avg = mean(tmp);
   [h sig ci] = ttest(tmp, 0.1);
   ci = (ci(2)-ci(1))/2;
   node_coverage = [node_coverage; data(ifirst, 1:cols-5), avg ci];
end

figure(1);
hold on;
x = [0:0.05:0.5]
y = 1-x
plot(x,y,'--r')
legend('DiSR','Ideal')
plot(node_coverage(:,1), node_coverage(:,2), symbol);
ylim([0 1])
xlabel('Bootstrap Node')
ylabel('node_coverage')



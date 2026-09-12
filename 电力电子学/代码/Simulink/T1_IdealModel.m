
%% 画 图
plot(out.Ipv(:,1),out.Ipv(:,2),'LineWidth',4) % standard
%% 设 置 图 片 位 置、 大 小 等 内 容
set(gca,'Position',[0.1 0.15 0.85 0.8]);
grid on;
set(gca, 'Fontsize', 40, 'Fontname', 'Times New Roman', 'Fontweight', 'Bold');
set(gca,'ygrid','on','gridlinestyle','--','Gridalpha',0.6);
set(gca,'xgrid','on','gridlinestyle','--','Gridalpha',0.6);
ylim([0,5]);
xlim([0,25]);
ylabel('Current(A)','FontSize',40, 'Fontname', 'Times New Roman', 'Fontweight', 'Bold');
xlabel('Voltage(V)','FontSize',40, 'Fontname', 'Times New Roman', 'Fontweight', 'Bold');
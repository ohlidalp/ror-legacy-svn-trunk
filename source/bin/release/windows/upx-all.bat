FOR /R . %%G IN (*.exe) DO upx %%G
FOR /R . %%G IN (*.dll) DO upx %%G

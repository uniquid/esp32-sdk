Import("env")
import shutil

#
# Dump build environment (for debug)
# print env.Dump()
#

# add /extern/hw/includeoverride include search path before the system ones
env['CPPPATH'] = [env['PROJECTSRC_DIR'] + '/extern/hw/includeoverride'] + env['CPPPATH']

print "Copy yajl header files"

shutil.rmtree('extern/hw/yajl', True)
shutil.copytree('extern/uidcore-c/yajl/src/api', 'extern/hw/yajl')

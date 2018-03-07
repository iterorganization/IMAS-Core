from MDSplus import *
import array
import numpy as np

def getTabs(tabs):
  line = ''
  for i in range(0, tabs):
    line = line + '   '
  return line
  


def dumpStruct(s, tabs):
  print getTabs(tabs) + s[0] + ':'
  for i in range(1, len(s)):
    if isinstance(s[i], Apd):
      if isinstance(s[i][0], Apd): #AoS
        dumpAoS(s[i], tabs+1)
      else:
        dumpStruct(s[i], tabs+1)
    else:
      if isinstance(s[i], TreeNode):
        print getTabs(tabs+1)+s[i].decompile()+':'
        if (s[i].getNumSegments() > 0 and (s[i].getDtype() == 'DTYPE_B' or s[i].getDtype() == 'DTYPE_BU')):
          dumpSlicedAoS(s[i], tabs+1)	 	
        else:		
          d = s[i].getData()
          if isinstance(d, Apd):
            print('INTERNAL ERROR: Reference canno be APD')
            return
          print getTabs(tabs+1) + d.decompile()
      else:
        if isinstance(s[i], Int8Array) or isinstance(s[i], Uint8Array):
          print getTabs(tabs+1) + '"'+array.array('B', s[i].data()).tostring()+'"'
        else:
          print getTabs(tabs+1) + s[i].decompile()
    #endif
  #endfor


def dumpAoS(s, tabs):
  for i in range(0, len(s)):
    print getTabs(tabs)+'['+str(i)+']'
    dumpStruct(s[i], tabs+1)

def dumpSlicedAoS(n, tabs):
    aosArr = []
    numSegments = n.getNumSegments()
    for i in range(0, numSegments):
      s = n.getSegment(i)
      if np.isscalar(s.getDimensionAt(0).data()):
        dim = 1
      else:
        dim = len(s.getDimensionAt(0).data())
      serialized = s.data()
      if(dim == 1):
        try:
            aosArr.append(Data.deserialize(serialized))
        except:
            aosArr.append(Data.deserialize(serialized[4:]))
      else:
        idx = 0
        buf = np.getbuffer(serialized)
        for j in range(0, dim):
          currSize = (np.frombuffer(buf, dtype = 'uint32', offset = idx, count = 1))[0]
          idx = idx+4
          currSerialized = np.frombuffer(np.getbuffer(buf, offset = idx, size = currSize), dtype='uint8')
          aosArr.append(Data.deserialize(currSerialized))
          idx = idx + currSize
    for sliceIdx in range(0, len(aosArr)):
        print getTabs(tabs+1) + '['+str(sliceIdx)+']'
        dumpStruct(aosArr[sliceIdx], tabs+2)


def dump(s):
  if(isinstance(s, Apd)):
    dumpAoS(s, 0)
  else:
    print s
    
    
    
    
    
    
    
    
    
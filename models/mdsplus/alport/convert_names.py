from MDSplus import *


def recTraverse(node):
  print node
  mm = node.getMembers()
  cc = node.getChildren()
  if len(mm) > 0 and mm[0].getName() == 'STATIC0' and len(cc) > 0 and cc[0].getName() == 'TIMED_10':
      print 'CONVERT!!'
      mm[0].rename('static')
      idx = 1
      print len(cc)
      for c in cc:
        c.rename('xtimed_'+str(idx))
        idx = idx + 1
      idx = 1
      for c in cc:
        c.rename('timed_'+str(idx))
        idx = idx + 1
        for mm in c.getMembers():
          if mm.getName() == 'AOS0':
            mm.rename('aos')
          else:
            if mm.getName() == 'TIME0':
              mm.rename('time')
      print 'FINITO'
      return
  members = node.getMembers()
  for member in members:
    recTraverse(member)
  children = node.getChildren()
  for child in children:
    recTraverse(child)
      
t = Tree('ids',-1, 'EDIT')
recTraverse(t.getNode('\IDS::TOP'))
t.write()
t.close()





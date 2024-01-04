#!/usr/bin/env python
import xml
import xml.sax
import os
from subprocess import call
from MDSplus import *
import numpy as np

isHomogeneousTime = False
homogeneousTime = ''

failedNames = []
missedNodes = []

oldToTrueMap = {}


def name2MDS(name):
  length = len(name)
  s = 0
  lowName = name.lower()
  for i in range(8,length):
    s = s + ord(lowName[i])
  outName = lowName[:8]
  outName = outName+str(s)
  return outName


def path2MDS(path):
  outPath = ''
  path1 = path.split('.')
  for currPath1 in path1:
    outPath = outPath + '.'
    path2 = currPath1.split(':')
    first2 = True
    for currPath2 in path2:
      if first2:
        first2 = False
      else:
        outPath = outPath + ':'
      outPath = outPath + name2MDS(currPath2)
  return outPath


def name2MDSNEW(inName, isMember):
  if inName == 'static0':
    return 'static'
  if inName == 'aos':
    return inName
  if inName.startswith('timed_'):
    return inName
  if inName.startswith('group_'):
    return inName
  if inName.startswith('item_'):
    return inName
  currPos = 0
  outName = ''
  if inName.isdigit():
    return inName
  try:
    name = oldToTrueMap[inName.lower()]
  except:
#    print ('Cannot map ' + inName.lower())
#    pass
    return ''
  for i in range(0,int(len(name)/12)+1):
    if i > 0 and currPos < len(name):
      if i == int(len(name)/12) and isMember:
        outName = outName + ':'
      else:
        outName = outName + '.'
    outName = outName + name[currPos:currPos + 12]
    currPos = currPos + 12
  return outName


def path2MDSNEW(inPath):
  prePath = ''
  if inPath.startswith('\IDS::TOP.'):
    prePath = '\IDS::TOP'
    path = str(inPath)[10:]
  else:
    path = str(inPath)
  outPath = ''
  path1 = path.split('.')
  for currPath1 in path1:
    outPath = outPath + '.'
    path2 = currPath1.split(':')
    first2 = True
    for currPath2 in path2:
      if first2:
        first2 = False
        outPath = outPath + name2MDSNEW(currPath2, False)
      else:
        if len(currPath2) > 12:
          outPath = outPath + '.'
        else:
          outPath = outPath + ':'
        outPath = outPath + name2MDSNEW(currPath2, True)
  return prePath+outPath
 

class Handler(xml.sax.ContentHandler):
  def __init__(self):
    print("INIT")
    self.namesStack = []
    self.paths = []
    self.oldToTrueNames = {}
  def addPaths(self):
    currPath = ''
    for name in self.namesStack:
      currPath = currPath + name
      self.paths.append(currPath)
     
  def startElement(self, tag, attributes):
    if tag == 'member':
      #print('MEMBER', attributes['NAME'])
      #self.namesStack.append(':'+attributes['NAME'])
      #self.addPaths()
      self.oldToTrueNames[name2MDS(attributes['NAME'])]=attributes['NAME']
    elif tag == 'node': 
#      print('NODE', attributes['NAME'])
      #self.namesStack.append('.'+attributes['NAME'])
      #self.addPaths()
      self.oldToTrueNames[name2MDS(attributes['NAME'])]=attributes['NAME']
  #def endElement(self, tag):
    #if tag == 'member' or tag == 'node':
    #  self.namesStack = self.namesStack[:-1]




####################Creation of the mappings for old/new paths in the pulse file####################################
def createMap():
  global oldToTrueMap
  h = Handler()
  xml.sax.parse(os.environ['IMAS_PREFIX']+'/models/mdsplus/alport/ids.xml', h)  
  oldToTrueMap = h.oldToTrueNames
  return h.oldToTrueNames


#####################################################################################################################



def convertData(d):
#  print 'CONVERT DATA '+d
  if isinstance(d, StringArray):
    dd = d.data()
#    print 'CONVERT ', dd
    leng = len(dd[0])
    print (leng, len(dd))
#    outD = np.empty([len(dd),leng], dtype = int)
    outD = np.empty([leng,len(dd)], dtype = int)
#    print leng, len(dd)
    for idx in range(0, len(dd)):
#      print 'IDX ', idx
      s = dd[idx][0:leng]
#      print 's', s
#      outD[idx,:] = [ord(i) for i in s]
      outD[:,idx] = [ord(i) for i in s]
    return Int8Array(outD)
  else:
    return d


def getTimeName(n):
#  print 'getTimeName', n.getSegmentDim(0)
  return str(n.getSegmentDim(0).getDescs()[0].getDescs()[2])


def convertTimedNode(inT, outT, inN, outN):
  global isHomogeneousTime
  global homogeneousTime
  nodeName = convertToLowPath(str(inN.getFullPath()))
  outN.deleteData()
  if isHomogeneousTime:
    timePath=homogeneousTime
  else:
    timePath = getTimeName(inN)
##WORKAROUND FOR WRONG TIMEBASE IN
    if len(str(inN.getFullPath())) > 18 and convertToLowPath(str(inN.getFullPath()))[-18:] == '.code0:output_f308':
      timePath = convertToLowPath((str(inN.getFullPath()))[:-18]+':time0')
#      print 'WORKAROUND  ', inN.getFullPath(), timePath
  if len(str(inN.getFullPath())) > 6 and convertToLowPath(str(inN.getFullPath()))[-6:] == ':time0':
    timePath = convertToLowPath((str(inN.getFullPath()))[:-6]+':time0')
#    print 'WORKAROUND 1 ', inN.getFullPath(), timePath
  
####
  timeNode = inT.getNode(timePath)
  timeSeg = timeNode.getSegment(0)
  timePath = path2MDSNEW(timePath)
#  print''
#  print '****  CONVERTED TIME: '+timePath+'  ****'
#  print''
  for segIdx in range(0, inN.getNumSegments()):
    s = inN.getSegment(segIdx)
    start = Data.compile('data(getSegment(build_path("'+timePath+'"),'+str(segIdx)+'))[0]', tree = outT)
    end = Data.compile('data(getSegment(build_path("'+timePath+'"),'+str(segIdx)+'))['+str(len(timeSeg.data()))+'-1]', tree = outT)
    dim = Data.compile('data(getSegment(build_path("'+timePath+'"),'+str(segIdx)+'))[0:'+str(len(timeSeg.data()))+'-1]', tree = outT)
#    end = Data.compile('data(getSegment(build_path("'+timePath+'"),'+str(segIdx)+'))['+str(len(timeSeg))+'-1]', tree = outT)
#    dim = Data.compile('data(getSegment(build_path("'+timePath+'"),'+str(segIdx)+'))[0:'+str(len(timeSeg))+'-1]', tree = outT)
#MAY 2019: for the last segment copy only meaningful samples, i.e. non 0 (except the first one)
    if segIdx < inN.getNumSegments() - 1:
      outN.makeSegment(start, end, dim, s.data())
    else:
      segTimes = inN.getSegmentDim(segIdx).data()
      if len(segTimes) > 0:
        numSamples = 0
#        while numSamples < len(segTimes):
 
        while numSamples < len(segTimes): 
          if(numSamples > 0 and segTimes[numSamples] == 0):
            break
          numSamples += 1
        #print('ULTIMO: ', s.data()[0:numSamples])
        print(inN, outN, numSamples)
        end = Data.compile('data(getSegment(build_path("'+timePath+'"),'+str(segIdx)+'))['+str(numSamples)+'-1]', tree = outT)
        dim = Data.compile('data(getSegment(build_path("'+timePath+'"),'+str(segIdx)+'))[0:'+str(numSamples)+'-1]', tree = outT)
        outN.makeSegment(start, end, dim, s.data()[0:numSamples])
      



def convertTimed(inT, outT, baseNodeName):
  for n in inT.getNodeWild(baseNodeName+'***', 'SIGNAL'):
    if n.getNumSegments() > 0 :
#      print(n)
      try:
        outN = outT.getNode(path2MDSNEW(str(n)))
        convertTimedNode(inT, outT, n, outN)
      except:
        print('Node '+str(n)+'  not found')  


def showAos(t, baseNodeName):
  for n in t.getNodeWild(baseNodeName+'***', 'NUMERIC'):
    name  = str(n)
    if name.endswith('STATIC0'):
      print(name)

      
def convertToLowPath(name):
  if name.startswith('\IDS::TOP'):
    name = name[10:]
  return name.lower()


def recBuildTimedMap(n, baseNodeName, currTimedIdx, timedMap):
#  print(n)
  descend = n.getDescendants()
  if len(descend) == 0:
#    if n.getDType() == 'SIGNAL' and n.getNumSegments() > 0 :
    if n.getNumSegments() > 0 :
      if n.getDtype() == 'DTYPE_B':
	# Aos Node
        timedMap[0][convertToLowPath(str(n))] = baseNodeName+'.timed_aos.group_'+str(1+int((currTimedIdx[0]-1)/1000))+'.item_'+str(currTimedIdx[0])
        currTimedIdx[0] = currTimedIdx[0]+1
      else:
        # Data Node
#        timedMap[convertToLowPath(str(n))] = baseNodeName+'.'+name2MDS('timed_'+str(currTimeIdx))
        timedMap[1][convertToLowPath(str(n))] = baseNodeName+'.timed_data.group_'+str(1+int((currTimedIdx[1]-1)/1000))+':item_'+str(currTimedIdx[1])
        currTimedIdx[1] = currTimedIdx[1]+1
  else:
    if len(descend) > 1 and descend[-1].name == 'SHAPE_OF0':
      try:
        numAos = descend[-1].data()
      except:
        numAos = 0
      for i in range(1, numAos+1):
        for nd in descend:
          if nd.name == str(i):
            currTimedIdx = recBuildTimedMap(nd, baseNodeName, currTimedIdx, timedMap)
    else:
        for currN in descend:
          currTimedIdx = recBuildTimedMap(currN, baseNodeName, currTimedIdx, timedMap)
  return currTimedIdx


def buildTimedMap(t, baseNodeName):
  currTimedIdx = [1, 1]
  timedMap = [{}, {}]
  n = t.getNode(baseNodeName)
  recBuildTimedMap(n, baseNodeName, currTimedIdx, timedMap)
  return timedMap


def convertTimedNodeAos(inT, outT, inN, outN, timedMap):
  global isHomogeneousTime
  global homogeneousTime
#  print 'CONVERT TIMED AOS homogeneousTime ' + str(isHomogeneousTime)
  outN.deleteData()
  if isHomogeneousTime:
    convTimePath = homogeneousTime
  else:
    timePath = convertToLowPath(getTimeName(inN))
    try:
      # This must be a Data node
#      convTimePath = timedMap[timePath.replace(':','.')]+':aos0'
      convTimePath = timedMap[1][timePath]
    except:
      print('Cannot map '+timePath)
      print('Trying '+timePath)
      convTimePath = timePath
  
  
  convTimePath = path2MDSNEW(convTimePath)
#  print''
#  print '****  CONVERTED TIME AOS: '+convTimePath+'  ****'
#  print''

  timeSeg = inN.getSegment(0)
  for segIdx in range(0, inN.getNumSegments()):
   # print('SCRIVO SEGMENTO ' + str(segIdx))
    s = inN.getSegment(segIdx)
    start = Data.compile('data(getSegment(build_path("'+convTimePath+'"),'+str(segIdx)+'))[0]', tree = outT)
    end = Data.compile('data(getSegment(build_path("'+convTimePath+'"),'+str(segIdx)+'))['+str(len(timeSeg.data()))+'-1]', tree = outT)
    dim = Data.compile('data(getSegment(build_path("'+convTimePath+'"),'+str(segIdx)+'))[0:'+str(len(timeSeg.data()))+'-1]', tree = outT)
#    end = Data.compile('data(getSegment(build_path("'+convTimePath+'"),'+str(segIdx)+'))['+str(len(timeSeg))+'-1]', tree = outT)
#    dim = Data.compile('data(getSegment(build_path("'+convTimePath+'"),'+str(segIdx)+'))[0:'+str(len(timeSeg))+'-1]', tree = outT)
    
#JULY 2019: for the last segment copy only meaningful samples, i.e. non 0 (except the first one)
    if segIdx < inN.getNumSegments() - 1:
      outN.makeSegment(start, end, dim, s.data())
    else:
      segTimes = inN.getSegmentDim(segIdx).data()
      if len(segTimes) > 0:
        numSamples = 0
#        while numSamples < len(segTimes):
 
        while numSamples < len(segTimes): 
          if(numSamples > 0 and segTimes[numSamples] == 0):
            break
          numSamples += 1
        #print('ULTIMO: ', s.data()[0:numSamples])
        #print('ULTIMO: ', s.data()[0:numSamples])
        print(inN, outN, numSamples)
        end = Data.compile('data(getSegment(build_path("'+convTimePath+'"),'+str(segIdx)+'))['+str(numSamples)+'-1]', tree = outT)
        dim = Data.compile('data(getSegment(build_path("'+convTimePath+'"),'+str(segIdx)+'))[0:'+str(numSamples)+'-1]', tree = outT)
        outN.makeSegment(start, end, dim, s.data()[0:numSamples])
    
#    outN.makeSegment(start, end, dim, s.data())


def convertStructItem(inT, inN, outT, timedMap, nameMap):
#  print 'CONVERT STRUCT ITEM '
#  print inN.getPath() + '   '+nameMap[inN.getName().lower()]
  outApd = Apd((),24)
  try:
    outApd.append(String(nameMap[inN.getName().lower()]))
  except:
    failedNames.append(inN.getName().lower())
    outApd.append(String(inN.getName().lower()))
  if inN.getUsage() == 'SIGNAL' and inN.getNumSegments() > 0:
    # This must be a Data node
    nameN = timedMap[1][convertToLowPath(str(inN))]
    try:
      outN = outT.getNode(path2MDSNEW(nameN))
    except:
      print ('Cannot find node 1' + nameN)
      print ('Cannot find node 2' + path2MDSNEW(nameN))
      print ('Trying: '+ str(inN))
      outN = outT.getNode(path2MDSNEW(str(inN)))
#    print('ORA SCRIVO IN ', outN)
    convertTimedNodeAos(inT, outT, inN, outN, timedMap)
    outApd.append(outN)
  else:
    if inN.getUsage() == 'NUMERIC' and inN.getNumSegments() > 0:
      print('INTERNAL ERROR, UNEXPECTED sub AoS')
      return None
    else:
      try:
        outApd.append(convertData(inN.getData()))
      except:
        return None
#  print('CONVERT STRUCT ITEM: ', outApd)
  return outApd


def convertSubAoS(inT, inN, inTimedN, outT, timedMap, nameMap):
  outApd = Apd((),24)
  outApd.append(String(nameMap[inN.getName().lower()]))
  if inTimedN.getUsage() != 'NUMERIC' or inTimedN.getNumSegments() == 0 or inTimedN.getName() != 'TIMED0':
    if not inTimedN.getNumSegments() == 0:
      print('INTERNAL ERROR: WRONG sub AoS for '+inTimedN.decompile() +' . Usage:'+inTimedN.getUsage()+' numSegments: ' + str(inTimedN.getNumSegments())+ '  name: '+inTimedN.getName())
    return None
  # This must be an Aos node
  outName = path2MDSNEW(timedMap[0][convertToLowPath(str(inTimedN))])
  print(inN.getPath()+':  AoS Type 3 '+ outName+':aos')
  outN = outT.getNode(outName+':aos')
#  outApd.append(outN)
  inPulse = inT.shot
  outPulse = outT.shot
  inName = str(inTimedN)
#  command = './convert_aos '+str(inPulse) + '  '+ str(inName)[10:] + '  '+str(outPulse)+'  '+str(outName)[10:]
  command = os.environ['IMAS_PREFIX']+'/models/mdsplus/alport/convert_aos '+str(inPulse) + '  '+ str(inName)[10:] + '  '+str(outPulse)+'  '+str(outName)[10:]
  print(command)
  os.system(command)
##Gabriele June 2019: append to outApd only if timed node has actually been written by convert_aos (in order to avoid reporting empty AoS)
#  if outN.getLength() > 0: 
#    outApd.append(outN)
#  else:
#    return None
#Report in any case the empty AoS, noe supported by lowlevel
  outApd.append(outN)
################################################################## 
  
  
  
  return outN



def convertStruct(inT, inN, outT, timedMap, nameMap, isTop):
#  print('CONVERT STRUCT: ', inN)
  descends = inN.getDescendants()
  if len(descends) == 0:
    retAos = convertStructItem(inT, inN, outT, timedMap, nameMap)
    return retAos
  else:   #check for sub AoS
    if(len(descends) == 3 and descends[0].getName() == 'NON_TIME100' and descends[1].getName() == 'TIME0' and descends[2].getName() == 'TIMED0'):
      currAosApd = Apd((),24)
      currAosApd.append( String(nameMap[inN.getName().lower()]))
      retAoS = convertSubAoS(inT, inN, descends[2], outT, timedMap, nameMap)
      if retAoS == None:
          return None
      currAosApd.append(retAoS)
#      print 'Appendo ', currAosApd
      return currAosApd
    else:
      outApd = Apd((),24)
#      print inN.getName()
      try:
        outApd.append( String(nameMap[inN.getName().lower()]))
      except:
        failedNames.append(inN.getName().lower());
        outApd.append(String(inN.getName().lower()))
      if descends[0].name == '1':
        aosApd = Apd((),24)
        try:
          numAos = descends[len(descends) - 1].data()  #SHAPE_OF contains the actual number of Aos elements
        except:
#Gabriele Jan 2019 Keep empty AoS
          numAos = -1
        if numAos == -1:
          return None
        empty = True
        for i in range(1, numAos+1):
          for nd in descends:
            if nd.name == str(i):   #find the node corresponding to the index
              currAosApd = Apd((),24)
              currAosApd.append( String(nameMap[inN.getName().lower()]))
              empty1 = True
              for n in nd.getDescendants():
#                print('CHIAMO AOS CONVERT STRUCT: ', n)
                currApd = convertStruct(inT, n, outT, timedMap, nameMap, False)
                if type(currApd) != type(None):
                  currAosApd.append(currApd)
                  empty1 = False
                  empty = False
              if not empty1:
                aosApd.append(currAosApd)
              else:
                aosApd.append(EmptyData)
#                print('AGGIUNGO AOS ', currAosApd)
#Gabriele Jan 2019 keep empty AoS fields        
#	if empty:
#          return None
        if isTop:
          return aosApd
        outApd.append(aosApd)
        return outApd
#      print('OTTENGO AOS', outApd)
      else:
#        outApd.append(String(nameMap[inN.getName().lower()]))   GABGAB
        empty = True
        for n in descends:
#        print('CHIAMO CONVERT STRUCT: ', n)
          currApd = convertStruct(inT, n, outT, timedMap, nameMap, False)
          if type(currApd) != type(None):
#          print('AGGIUNGO ', n, currApd)
            outApd.append(currApd)
#          print('OTTENGO ', outApd)
            empty = False
        if empty:
          return None
#    print('RESTITUISCO AOS ', outApd)
    return outApd





def convertAoS(inT, inName, outT, nameMap, idsName):
  timedMap = buildTimedMap(inT, inName)
#Add reference to time fields below outside AoS
#  timeMap1 =  buildTimedMap(inT, idsName) 
#  for path in timeMap1:
#    if not(path in timeMap):
#      timeMap[path] = timeMap1[path]
#################
  inN = inT.getNode(inName)
  try:
    outStatic = outT.getNode(path2MDSNEW(inName+':static0'))
    outStatic.deleteData()
  except:
    print('Warning: output node '+  inName+':static0  Not Found')
#    print('Warning: output node '+  inName+':static0 ('+path2MDSNEW(inName+':static0')+')  Not Found')
    return
  outStatic.putData(convertStruct(inT, inN, outT, timedMap, nameMap, True))



def convertIdsRec(inT, outT, idsNode, nameMap, idsName):
    descends = idsNode.getDescendants()
    if len(descends) == 0:
      if idsNode.getNumSegments() > 0: 
        if idsNode.getUsage() != 'SIGNAL' and idsNode.getUsage() != 'NUMERIC':
          print("INTERNAL ERROR IN CONVERT IDS: non signal segmented node encountered")
          return
#        print idsNode.getPath()
        try:
          outN = outT.getNode(path2MDSNEW(idsNode.getPath()))
        except:
#          print ('Warning: output node '+  idsNode.getPath()+' not found')
          print('Warning: output node '+  idsNode.getPath()+'  ('+path2MDSNEW(idsNode.getPath())+')  not found')
          missedNodes.append(idsNode.getPath())
          return
        convertTimedNode(inT, outT, idsNode, outN)
      else:
        try :
          outN = outT.getNode(path2MDSNEW(idsNode.getPath()))
          if idsNode.getUsage() == 'TEXT':
            outN.putData(convertData(idsNode.getData()))
          else:
            outN.putData(idsNode.getData())
        except:
          pass
    else: #There are descendants: check for AoS
      if descends[len(descends)-1].getName() == 'SHAPE_OF0': #Type 1 AoS
        print('Type 1 AoS: '+idsNode.getPath())
        convertAoS(inT, idsNode.getFullPath(), outT, nameMap, idsName)
      else:
        if(len(descends) == 3 and descends[0].getName() == 'NON_TIME100' and descends[1].getName() == 'TIME0' and descends[2].getName() == 'TIMED0'):  #type 3 AoS
          print('Type 3 AoS: '+idsNode.getPath())
          inPulse = inT.shot
          outPulse = outT.shot
          inName = str(idsNode.getPath())
          outName = path2MDSNEW(inName+'.timed_aos.group_1:item_1')
          command = os.environ['IMAS_PREFIX']+'/models/mdsplus/alport/convert_aos '+str(inPulse) + '  '+ inName[10:]+':TIMED0' + '  '+str(outPulse)+'  '+outName[10:]
 #         command = 'convert_aos '+str(inPulse) + '  '+ inName[10:]+':TIMED0' + '  '+str(outPulse)+'  '+outName[10:]
          print(command)
          os.system(command)
#Copy also content of node TIMED0
          convertIdsRec(inT, outT, descends[1], nameMap, idsName)
        else:
          for n in descends:
            convertIdsRec(inT, outT, n, nameMap, idsName)


def convertIds(inPulse, outPulse, idsName, nameMap):
  global isHomogeneousTime
  global homogeneousTime
  inT = Tree('ids', inPulse)
  outT = Tree('ids', outPulse)
#check for homogeneous time  
  try:
    homoNid = inT.getNode(idsName + '.IDS_PROP652:HOMOGENE869')
    if homoNid.data() == 1:
      print ('HOMOGENEOUS TIME')
      isHomogeneousTime = True
      homogeneousTime = idsName+':time0'
    else:
      print ('NON HOMOGENEOUS TIME')
      isHomogeneousTime = False
      homogeneousTime = ''
  except:
    print ('MISSING HOMOGENEOUS SPEC')
    isHomogeneousTime = False
    homogeneousTime = ''
    return
##############################
  convertIdsRec(inT, outT, inT.getNode(idsName), nameMap, idsName)


def convertAll(inPulse, outPulse, nameMap):
  inT = Tree('ids', inPulse)
  outT = Tree('ids', outPulse)
  top = inT.getNode('\ids::top')
  descendants = top.getDescendants()
  for n in descendants:
    print('*************  '+str(n.getName())+'   ****************')
    convertIds(inPulse, outPulse, n.getName(), nameMap)


def getPath(p):
  if p[0]=='~':
    return os.path.expanduser(p)
  else:
    return os.path.expandvars(p)


###############MAIN
import sys
from subprocess import call
from MDSplus import *
from glob import glob
import re

if len(sys.argv) != 5:
  print ('Usage: python alport.py <pulse> <run> <oldDir> < newDir>')
  print ('Replace pulse and run number with "all" to convert all pulsefiles from oldDir')
else:
  pulse = sys.argv[1]
  run = sys.argv[2]
  oldDir = getPath(sys.argv[3])
  newDir = getPath(sys.argv[4])

  PFlist = list()
  listAll = [re.split('_|\.',f.split('/')[-1])[1] for f in glob(oldDir+'/ids_*.datafile')]
  if pulse=='all' and run=='all':
    PFlist = [(int(s[:-4]),int(s[-4:])) if len(s)>4 else (0,int(s)) for s in listAll]
  elif run=='all':
    for s in listAll:
      if len(s)<5 or re.match(pulse+'\d{4}$',s):
        PFlist.append((int(pulse),int(s[-4:])))
  elif pulse=='all':
    for s in listAll:
      if re.match('\d*'+run+'$',s) and len(s)>4:
        PFlist.append((int(s[:-4]),int(run)))
      elif re.match('^0{,2}'+run+'$',s): 
        PFlist.append((0,int(run)))
  else:
    PFlist = [(int(pulse),int(run))]

  for pf in PFlist:
    #reset these for safety?
    isHomogeneousTime = False
    homogeneousTime = ''
    failedNames = []
    missedNodes = []
    oldToTrueMap = {}

    actPulse = pf[1]+pf[0]*10000
    actPulseStr = str(actPulse).zfill(3)

    print(oldDir+'/ids_'+actPulseStr+'.tree')
    if not os.path.isfile(oldDir+'/ids_'+actPulseStr+'.datafile'):
      print('File does not exist, stop.')
      sys.exit(1)

    os.system('cp '+ oldDir+'/ids_'+actPulseStr+'.* .')
    os.environ['ids_path'] =  os.getcwd()
    map = createMap()
    os.system('cp -f '+os.environ['IMAS_PREFIX']+'/models/mdsplus/ids_model.* .')
    print(os.getcwd())
    print(os.environ['ids_path'])
    t = Tree('ids', -1)
    t.createPulse(123)
    convertAll(actPulse, 123, map)
    os.system('cp ids_123.tree '+newDir+'/ids_'+actPulseStr+'.tree')
    os.system('cp ids_123.characteristics '+newDir+'/ids_'+actPulseStr+'.characteristics')
    os.system('cp ids_123.datafile '+newDir+'/ids_'+actPulseStr+'.datafile')
    os.system('rm ids_123.*')
    os.system('rm -f ids_'+actPulseStr+'.*')
    print('INCONSISTENT ITEMS:', failedNames)
    print('NODES NOT DEFINED IN MODEL: ', missedNodes)


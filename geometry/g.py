from shutil import copyfile
header="/home/mfila/TPCReco/resources/header_test_geometry_elitpc.dat"
output1="filtered0AStrips.txt"
#output2="filteredBStrips.txt"

copyfile(header, output1)
#copyfile(header, output2)
known=set()
sect={"-":"0","A":"1","B":"2"}
body="/home/mfila/TPCReco/resources/raw_test_geometry_elitpc.txt"
h=open(header)
w1=open(output1,"a")
#w2=open(output2,"a")
with open(body) as r:    
   #r=f.readlines()[30:]
  for i,line in enumerate(r,start=1): 
        s=line.split()
        v=[s[0],sect[s[3]],s[2], "0", s[8], s[9],s[11],s[12],s[13],s[14]] 
        index=s[9]+s[11]
       # print(s[0],s[3])
        if True:
        
        #if s[0]=="W":   
         #     v=[s[0],str(i), "0", s[8], s[9],s[11],s[12],str(-float(s[13])),s[14]]  
             #pass
            w1.write("\t\t".join(v)+"\n")
        #if (s[0]=="V") and ((s[3]=="B") or (s[3]=="-")):
         #  w2.write("\t".join(v)+"\n")
  #         known.add(index)
print(output1+" written")

           

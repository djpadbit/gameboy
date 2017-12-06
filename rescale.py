import math
x = [250]*160
y = [250]*144
def resizePixels(w1,h1,w2,h2):
	x_ratio = float(w1)/float(w2)
	y_ratio = float(h1)/float(h2) 
	for i in range(h2):
		for j in range(w2):
			px = math.floor(j*x_ratio)
			py = math.floor(i*y_ratio)
			x[int(px)] = j
			y[int(py)] = i
			#print int(px),i,int(py),j
			#temp[(i*w2)+j] = pixels[(int)((py*w1)+px)]
	#return temp

resizePixels(160,144,70,63)
at = 0
string = "unsigned char scalearrx["+str(len(x))+"] = {"
stl = len(string)
for i in range(len(x)):
	string += str(x[i])
	if i != len(x)-1 and at != 30:
		string += ", "
	elif at == 30:
		string += ",\n"+" "*stl
		at = 0
	at += 1
string += "};"
print string

at = 0
string = "unsigned char scalearry["+str(len(y))+"] = {"
stl = len(string)
for i in range(len(y)):
	string += str(y[i])
	if i != len(y)-1 and at != 30:
		string += ", "
	elif at == 30:
		string += ",\n"+" "*stl
		at = 0
	at += 1
string += "};"
print string
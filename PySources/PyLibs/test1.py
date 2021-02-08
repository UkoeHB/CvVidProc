def testfunc(mydict, key, data):
	print('testing dictionary')
	print(type(mydict))

	mydict[key] = data
	if "num" in mydict:
		mydict["num"] = mydict["num"] + 1
	else:
		mydict["num"] = 1

	print(mydict)
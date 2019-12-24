#ifndef TOPOCOMPUTE_H
#define TOPOCOMPUTE_H
#include <fstream>
#include <map>
#include <commonlib.h>
#include <TreeNode/activator.h>
#include <Node.h>
#include <cstdarg>

using namespace MATHLIB;

namespace TopoENV{
    void ProcessBracket(string shape_str, std::vector<int> &shape_v){
	    string inner_str = shape_str.substr(1, shape_str.size()-2); //leave out the left and right bracket
	    inner_str.append(",");
	    int start = 0;
	    for (int i = 0; i < inner_str.size(); i ++){
		    auto ch = inner_str[i];
		    if (ch == ','){
			    string num = inner_str.substr(start, i - start);
			    if (num == "?"){
				    shape_v.push_back(-1);
			    }else{
				    shape_v.push_back(stoi(num));
			    }
			    start = i + 1; // skip the possible blank
			    i ++; // skip to the next number
		    }
	    }//end for
        if (shape_v.size() == 1){
            shape_v.push_back(1);
        }
    }

    template<typename T>
    MULTINode<T> ConstructTree(string filepath, map<string, MULTINode<T>>& map_s_n){
	    JsonReader reader;
	    JsonValue froot;
	    string colon = ":";
	    MULTINode<T> root;
	    ifstream in(filepath, ios::binary);
	    if (!in.is_open()){
		    cout << "Error opening file\n";
		    return root;
	    }
	    if (reader.parse(in, froot)){
		    for (int i = 0; i < froot.size(); i ++){
			    string step_name = "step";
				step_name.append(to_string(i));
			    JsonValue sub_root = froot[step_name]; 
                cout << "#####" << step_name << "#####" << endl;
			    JsonValue::Members op_name = sub_root.getMemberNames(); //for each node name in stepi
			    for (auto iter = op_name.begin(); iter != op_name.end(); iter ++){
				    JsonValue node = sub_root[*iter];
				    string new_node_name = step_name;
				    new_node_name.append(colon);
				    new_node_name.append(*iter);
                    cout << "*****" << new_node_name << "*****" << endl;
                    string op = node["OPERATION"].asCString();
				    int dimension = 1; // default dimension is 1
				    std::vector<int> shape;
				    if (!node["SHAPE"].isNull()){
					    ProcessBracket(node["SHAPE"].asCString(), shape);
					    dimension = shape.size();
				    }
                    TensorData<T> temp_data(tyname::D_0);
                    if (dimension == 2){
                        if (shape[1] == 1){
                            temp_data.setType(tyname::D_1);
                            Eigen_Vector<T>& s_data = temp_data.E1D;
                            if (!node["TENSOR_VALUE"].isNull()){s_data.setData(shape[0], node["TENSOR_VALUE"]);}
                        }else{
                            temp_data.setType(tyname::D_2);
                            Eigen_2D<T>& s_data = temp_data.E2D;
                            if (!node["TENSOR_VALUE"].isNull()){s_data.setData(shape[0], shape[1], node["TENSOR_VALUE"]);}
                        }
                    }else if (dimension == 3){
                        temp_data.setType(tyname::D_3);
                        Eigen_3D<T>& s_data = temp_data.E3D;
                        if (!node["TENSOR_VALUE"].isNull()){s_data.setData(shape[0], shape[1], shape[2], node["TENSOR_VALUE"]);}
                    }else if (dimension == 4){
                        temp_data.setType(tyname::D_4);
                        // cout << (temp_data.getType() == tyname::D_4) << endl;
                        //cout << temp_data.getType() << endl;
                        Eigen_4D<T>& s_data = temp_data.E4D;
                        if (op == "Identity"){
                            //which means this is kernel matrix
                            if (!node["TENSOR_VALUE"].isNull()){s_data.setData(shape[0], shape[1], shape[2], shape[3], node["TENSOR_VALUE"], matrix_type::kernel);}
                        }
                    }
				    std::vector<string> INPUT;
				    for (int i = 0; i < node["INPUT"].size(); i ++){
					    INPUT.push_back(node["INPUT"][i].asCString());
				    }//end for
				    string type = node["TYPE"].asCString();
                    string father_name = node["FATHER"].isNull()? "None" : node["FATHER"].asCString();
				    MULTINode<T> newnode(temp_data, father_name, shape, *iter, op, INPUT, INPUT.size());
				    root.reset(newnode);
                    //store the address of this object
				    map_s_n.insert(make_pair(*iter, newnode));
			    }
		    }//end for
	    }
	    return root;
    }

    template<typename T>
	void ComputeAll(map<string, MULTINode<T>> &indgree, MULTINode<T> &tnode){
        std::map<string, int> namemap;
        std::map<int, tyname> int_name;
        namemap.insert(make_pair("Add", 1));
        namemap.insert(make_pair("MatMul", 2));
        namemap.insert(make_pair("Softmax", 3));
        namemap.insert(make_pair("Reshape", 4));
        namemap.insert(make_pair("Sigmoid", 5));
        namemap.insert(make_pair("Relu", 6));
        namemap.insert(make_pair("Conv2D",7));
        int_name.insert(make_pair(1, tyname::D_1));
        int_name.insert(make_pair(2, tyname::D_2));
        int_name.insert(make_pair(3, tyname::D_3));
        int_name.insert(make_pair(4, tyname::D_4));
        TensorData<T> input_1;
        TensorData<T> input_2;
        TensorData<T> output;
        std::vector<int> new_shape;
        Eigen_Vector<T> final_res;
        cout << tnode.op << " ";
        tyname type_name;
        switch(namemap[tnode.op]){
            case 1:
                cout << "1" << endl;
                input_1 = indgree[tnode.children[0]].data;
                input_2 = indgree[tnode.children[1]].data;
                type_name = input_1.getType();
                if (type_name == tyname::D_2){
                    //D_2
                    if (input_1.E2D.get_col_length() == input_2.E2D.get_col_length() || input_1.E2D.get_row_length() == input_2.E2D.get_row_length()){
                        output.setData(input_1.E2D.AddWithoutBroadCast(input_2.E2D));
                    }else{
                        output.setData(input_1.E2D.AddBoradCast(input_2.E1D));
                    }
                    new_shape.push_back(output.E2D.get_row_length());
                    new_shape.push_back(output.E2D.get_col_length());
                    tnode.setData(output);
                    tnode.setShape(new_shape);
                }else{
                    //D_4
                    output.setData(input_1.E4D.AddBoradCast(input_2.E1D));
                    output.getShape(&new_shape);
                    tnode.setData(output);
                    tnode.setShape(new_shape);
                }
                cout << "1 finished" << endl;
                break;
            case 2:
                cout << "2" << endl;
                input_1 = indgree[tnode.children[0]].data;
                cout << input_1.E2D.get_col_length() << "=====" << input_1.E2D.get_row_length() << endl;
                input_2 = indgree[tnode.children[1]].data;
                cout << input_2.E2D.get_col_length() << "=====" << input_2.E2D.get_row_length() << endl;
                output.setData(input_1.E2D.Matmul(input_2.E2D));
                new_shape.push_back(output.E2D.get_row_length());
                new_shape.push_back(output.E2D.get_col_length());
                tnode.setData(output);
                tnode.setShape(new_shape);
                cout << "2 finished" << endl;
                break;
            case 3:
                cout << "3" << endl;
                input_1 = indgree[tnode.children[0]].data;
                output.setData(input_1.E2D.softmax2d(true));
                new_shape.push_back(output.E2D.get_row_length());
                new_shape.push_back(output.E2D.get_col_length());
                tnode.setData(output);
                tnode.setShape(new_shape);
                final_res = output.E2D.Argmax(true);
                cout << "Final Result:" << endl;
                final_res.Printout();
                cout << "3 finished" << endl;
                break;
            case 4:
                /*
                [a, b, c, d]
                -a: Number of pictures 
                -b: x dimension
                -c: y dimension
                -d: Number of channels
                */
                cout << "4" << endl;
                input_1 = indgree[tnode.children[0]].data;
                output.setData(input_1.E4D.reshape());
                new_shape.push_back(output.E2D.get_row_length());
                new_shape.push_back(output.E2D.get_col_length());
                tnode.setData(output);
                tnode.setShape(new_shape);
                cout << "4 finished" << endl;
                break;
            case 5:
                cout << "5" << endl;
                input_1 = indgree[tnode.children[0]].data;
                type_name = input_1.getType();
                if (type_name == tyname::D_2){
                    output.setData(input_1.E2D.apply(MATHLIB::sigmoid<T>));
                    new_shape.push_back(output.E2D.get_row_length());
                    new_shape.push_back(output.E2D.get_col_length());
                    tnode.setData(output);
                    tnode.setShape(new_shape);
                }else{
                    output.setData(input_1.E4D.apply(MATHLIB::sigmoid<T>));
                    output.getShape(&new_shape);
                    tnode.setData(output);
                    tnode.setShape(new_shape);
                }
                cout << "5 finished" << endl;
                break;
            case 6:
                cout << "6" << endl;
                input_1 = indgree[tnode.children[0]].data;
                type_name = input_1.getType();
                if (type_name == tyname::D_2){
                    output.setData(input_1.E2D.apply(MATHLIB::relu<T>));
                    new_shape.push_back(output.E2D.get_row_length());
                    new_shape.push_back(output.E2D.get_col_length());
                    tnode.setData(output);
                    tnode.setShape(new_shape);
                }else{
                    output.setData(input_1.E4D.apply(MATHLIB::relu<T>));
                    output.getShape(&new_shape);
                    tnode.setData(output);
                    tnode.setShape(new_shape);
                }
                cout << "6 finished" << endl;
                break;
            default:
                cout << "Invalid Operation!!!" << endl;
        }
    }

    template<typename T>
    void TopoComputeEngine(map<string, MULTINode<T>> indgree, TensorData<T> const input){
        std::deque<string> q;
        //initialzied the queue with node of which the indegree in 0
        for (auto iter = indgree.begin(); iter != indgree.end(); iter ++){
            if (iter->second.getDegree() == 0){
                //store the value of iter
                q.push_back(iter->first);
            }
        }//end for
        while(q.size() > 0){
             int size_of_q = q.size();
             while (size_of_q > 0){
                 string nodename = q.front();
                 q.pop_front();
                 MULTINode<T>& tnode = indgree[nodename];
                 //update indgree and queue
                 if (tnode.father != "None"){
                    indgree[tnode.father].setDegree(indgree[tnode.father].getDegree() - 1);
                    if (indgree[tnode.father].getDegree() == 0){
                        q.push_back(tnode.father);
                    }
                 }
                 string op_name = tnode.op;
                 cout << "ABC " << op_name << " CBA" << endl;
                //  if (op_name == "Placeholder"){
                //      tnode.setData(input);
                //  }else if(op_name == "Identity"){
                //     // cout << tnode.data.getData() << endl;
                //  }else{
                //      ComputeAll<T>(indgree, tnode);
                //  }
                 size_of_q --;
             }
        }//end for outer while
    }
}

#endif
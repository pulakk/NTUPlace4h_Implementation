#include <iostream>
#include <string>
#include <bits/stdc++.h>
using namespace std;

typedef struct Block{
    int x, y; // x and y coordinates
    int w, h; // width and height
    bool is_macro;
    int id; // unique ids

    /*  constructor for
        macro block */
    Block(int x, int y, int w, int h):
    x(x),
    y(y),
    w(w),
    h(h),
    is_macro(true),
    id(-1){};

    /*  constructor for
        non macro block */
    Block(int w, int h):
    x(-1),
    y(-1),
    w(w),
    h(h),
    is_macro(false),
    id(-1){};
}Block;

class Node{
    public:
    Block* block = NULL;
    vector<Node*> children;

    /* assumes both blocks exist in the tree */
    Node* common_ancestor(Block* a, Block * b){
        if(block == a or block==b) return this; /* return current node if found */

        vector<Node*> nodes_found; /* always < 3 {only two blocks to find} */
        for(auto child:children){
            Node* tmp_node = child->common_ancestor(a, b); /* recursive call for each child */
            if(tmp_node!=NULL) nodes_found.push_back(tmp_node); /* if found something increment counter */
        }

        /* both nodes found => current is parent */
        if(nodes_found.size() == 2) return this; 

         /* one node found is either => 
                1. parent => return parent 
                2. a block node => return anything other than NULL
        */
        else if(nodes_found.size() == 1) return nodes_found[0];
        /* NULL indicates no nodes found */
        return NULL; 
    }

    /* find the relative
       height of a subnode */
    int height_node(Node* node, int h=1){
        if(node == this) return h;
        for(auto child:children){
            int tmp_h = child->height_node(node, h+1); /*  */
            if(tmp_h > 0) return tmp_h;
        }
        return -1;
    }
    

    /* height of lowest 
       common ancestor (LCA) */
    int lca_h(Block &a, Block &b){
        Node* ancestor = common_ancestor(&a, &b);
        if(ancestor==NULL) return -1;
        else return height_node(ancestor);
    }
};


int sq(int a){
    return a*a;
}

int distance(Block* a, Block* b){
    return sq(a->x - b->x) + sq(a->y - b->y); 
}

/* Routability driven analytical placement */
class RDAP{
    public:
    /* variables */
        vector<Block*> blocks;
        vector<Block*> macro_blocks;
        int n_hgs;
        Node* tree = NULL;
        vector<vector<int>> nets;

    /* blocks, nets and nodes */   
        void add_block(Block &block){
            if(block.is_macro && !(macro_blocks.size() == blocks.size()))
                cout<<"Cannot add macros after standard cells have been added";
            else{
                block.id = blocks.size(); // assign new id
                blocks.push_back(&block);
                if(block.is_macro) macro_blocks.push_back(&block);
            }
        }

        void connect(vector<int> ids){
            for(auto id:ids)if(id>blocks.size())return;
            nets.push_back(ids);
        }

        void add_node(Node* parent, Node* child, Block* block = NULL){
            if(parent == NULL) 
                if(tree == NULL) tree = child;
                else cout<<"Root Already Present";
            else parent->children.push_back(child);
            
            child->block = block;
        }

    /* calculating affinities */
        float get_affinity(vector<int> &HGs, Block &a, Block &b){            
            int e = 1; // number of nets connecting a and b
            int epsilon = 10; // epsilon parameter
            
            /* add sum of no. of pins 
               for each common net */
            for(auto pins:nets) 
                if(find(pins.begin(), pins.end(), a.id)!=pins.end()
                && find(pins.begin(), pins.end(), b.id)!=pins.end()) e+=pins.size(); 
                
            int A = a.h * a.w + b.h * b.w; // sum of areas 
            int k = tree->lca_h(a, b); // height of lowest common ancestor
            
            if(A==0 || e == 0) return -1;
            if(HGs[a.id] == HGs[b.id]) return float(epsilon * k) / float(A * e);
            return float(k) / float(A * e);
        }

    /* Hierarchy Group Functions */  
        /* Match macros
        that have same dimensions*/
        void dim_HG_grouping(vector<int> &HGs, int &n_hgs, int max_dst = 10){
            /* assigning HGs
             to macros */
            for(unsigned int i=0;i<macro_blocks.size();i++){
                if(HGs[i]==-1) HGs[i] = n_hgs++; // assign new group id

                for(unsigned int j=i+1;j<macro_blocks.size();j++){
                    if(blocks[i]->h == blocks[j]->h
                    && blocks[i]->w == blocks[j]->w
                    && distance(blocks[i], blocks[j]) < max_dst){
                        HGs[j] = HGs[i]; // set same group id
                    }
                }
            }
        }

        /* assigning HGs to standard cells and 
        unmatched macros based on hierarchy*/
        vector<int> hrchy_grouping(vector<int> &HGs, int &n_hgs, Node* cur_node){
            vector<int> ids;
            vector<int> tmp_ids;
 
            // return empty ids list
            if(cur_node==NULL) return ids; 
            // add current node to list of ids
            if(cur_node->block!=NULL) ids.push_back(cur_node->block->id); 

            for(auto child:cur_node->children){
                tmp_ids = hrchy_grouping(HGs, n_hgs, child); 
                for(auto tmp_block:tmp_ids)
                    ids.push_back(tmp_block); // add new block ids
                vector<int>().swap(tmp_ids); // clear memory
            }

            /* find an already 
            assigned macro group 
            in the list of ids */
            int macro_group = -1;
            int macro_id;
            for(unsigned int i=0;i<ids.size();i++)
                if(blocks[ids[i]]->is_macro){
                    if(HGs[ids[i]]!=-1){
                        macro_group = HGs[ids[i]];
                        macro_id = ids[i];
                    }
                }

            /* if found */
            if(macro_group != -1){
                /* assign that macro group
                id to all the other blocks 
                in the list of ids */
                for(unsigned int i=0;i<ids.size();i++)
                    if(!blocks[ids[i]]->is_macro || HGs[ids[i]]==-1)
                        HGs[ids[i]] = macro_group;
                vector<int>().swap(ids); // clear memory
                ids.push_back(macro_id);
            }

            return ids;
        }
        
        /* construct and return 
        the Hierarchy Groups */
        vector<int> get_HGs(){
            vector<int> HGs;
            int n_hgs = 0;
            HGs.resize(blocks.size(), -1);

            dim_HG_grouping(HGs, n_hgs); // macro based
            vector<int> ids = hrchy_grouping(HGs, n_hgs, tree); // hierarchy based 

            // set remaining HGs
            for(auto id:ids) if(HGs[id]==-1)HGs[id] = n_hgs++;
            ids.clear();
            vector<int>().swap(ids);

            return HGs;
        }

        void place(){
            /* DESIGN HIERARCHY IDENTIFICATION */
            vector<int> HGs = get_HGs();
            float affinity[blocks.size()][blocks.size()];

            cout<<"HGs: ";for(auto hg:HGs)cout<<hg<<" ";cout<<endl;        
            
            cout<<"\nMacro Block Distances\n";
            for(auto a:macro_blocks){
                for(auto b:macro_blocks)
                    cout<<distance(a, b)<<"\t";
                cout<<endl;
            }
            
            cout<<"\nLCA Heights\n";
            for(auto a:blocks){
                for(auto b:blocks)
                    cout<<tree->lca_h(*a, *b)<<"\t";
                cout<<endl;
            }

            for(auto a:blocks){
                for(auto b:blocks)
                    affinity[a->id][b->id] = get_affinity(HGs, *a, *b);
            }

            cout<<"\nAffinities\n";
            for(int i=0;i<blocks.size();i++){
                for(int j=0;j<blocks.size();j++){
                    cout<<fixed<<setprecision(2)<<affinity[i][j]<<" ";
                }
                cout<<endl;
            }
        }
};


int main(){
    // blocks
    Block A(0,0,2,1);
    Block B(2,0,2,1);
    Block C(2,3,2,2);
    Block a(1,1);
    Block b(1,1);
    Block c(1,2);
    Block d(1,1);
    Block e(1,1);

    // variables
    Node n_V, n_X, n_Y, n_Z, n_W; // inner nodes
    Node n_A, n_B, n_C; // non macro blocks - leaf nodes
    Node n_a, n_b, n_c, n_d, n_e; // blocks - leaf nodes


    // analytical placement - object
    RDAP rdap;

    /* add macro blocks first*/
    rdap.add_block(A); // 0
    rdap.add_block(B); // 1
    rdap.add_block(C); // 2
    /* then add standard blocks */
    rdap.add_block(a); // 3
    rdap.add_block(b); // 4
    rdap.add_block(c); // 5
    rdap.add_block(d); // 6
    rdap.add_block(e); // 7

    rdap.connect(vector<int> {{0,1,2,3}});
    rdap.connect(vector<int> {{0,3,4}});
    rdap.connect(vector<int> {{1,5,6,0}});
    rdap.connect(vector<int> {{2,1,6,7}});

    rdap.add_node(NULL, &n_X); /* parent node */
    rdap.add_node(&n_X, &n_Y);
    rdap.add_node(&n_X, &n_W);
    rdap.add_node(&n_Y, &n_Z);
    rdap.add_node(&n_Y, &n_V);
    rdap.add_node(&n_V, &n_A, &A); // leaf nodes
    rdap.add_node(&n_V, &n_a, &a); // contain block 
    rdap.add_node(&n_V, &n_b, &b); // references
    rdap.add_node(&n_Z, &n_B, &B);
    rdap.add_node(&n_Z, &n_c, &c);
    rdap.add_node(&n_Z, &n_d, &d);
    rdap.add_node(&n_W, &n_C, &C);
    rdap.add_node(&n_W, &n_e, &e);

    cout<<"Running Placement Algorithm ... \n\n";
    rdap.place();

    cout<<"\n... complete!\n";
    return 0;
} 